#include <iostream>                                                                                                                                                                                                                                                 
#include <vector>
#include <string>
#include <boost/filesystem.hpp> 
#include "util.hpp"


//将数据源的路径 和 清理后干净文档的路径 径定义好
const std::string src_path = "data/input";          //数据源的路径
const std::string output = "data/raw_html/raw.txt"; //清理后干净文档的路径
 
//DocInfo --- 文件信息结构体
typedef struct DocInfo
{
    std::string title;   //文档的标题
    std::string content; //文档的内容
    std::string url;     //该文档在官网当中的url
}DocInfo_t;
 
// const & ---> 输入
// * ---> 输出
// & ---> 输入输出
 
//把每个html文件名带路径，保存到files_list中
bool EnumFile(const std::string &src_path, std::vector<std::string> *files_list);
 
//按照files_list读取每个文件的内容，并进行解析
bool ParseHtml(const std::vector<std::string> &files_list, std::vector<DocInfo_t> *results);
 
//把解析完毕的各个文件的内容写入到output
bool SaveHtml(const std::vector<DocInfo_t> &results, const std::string &output);


bool EnumFile(const std::string &src_path, std::vector<std::string> *files_list)
{
    namespace fs = boost::filesystem;
    fs::path root_path(src_path); // 定义一个path对象，枚举文件就从这个路径下开始
    // 判断路径是否存在
    if(!fs::exists(root_path))
    {
        std::cerr << src_path << " not exists" << std::endl;
        return false;
    }
    
    // 对文件进行递归遍历
    fs::recursive_directory_iterator end; // 定义了一个空的迭代器，用来进行判断递归结束
    for(fs::recursive_directory_iterator iter(root_path); iter != end; iter++)
    {
        // 判断指定路径是不是常规文件，如果指定路径是目录或图片直接跳过
        if(!fs::is_regular_file(*iter))
        {
            continue;
        }
 
        // 如果满足了是普通文件，还需满足是.html结尾的
        // 如果不满足也是需要跳过的
        // ---通过iter这个迭代器（理解为指针）的一个path方法（提取出这个路径）
        // ---然后通过extension()函数获取到路径的后缀
        if(iter->path().extension() != ".html")
        {
            continue;
        }
 
        //std::cout << "debug: " << iter->path().string() << std::endl; // 测试代码
      
        // 走到这里一定是一个合法的路径，以.html结尾的普通网页文件
        files_list->push_back(iter->path().string()); // 将所有带路径的html保存在files_list中，方便后续进行文本分析
    }
    return true;
}


static bool ParseTitle(const std::string &file, std::string *title)
{
    std::size_t begin = file.find("<title>");
    if(begin == std::string::npos)
    {
        return false;
    }
 
    std::size_t end = file.find("</title>");
    if(end == std::string::npos)
    {
        return false;
    }
 
    begin += std::string("<title>").size();
    if(begin > end)
    {
        return false;
    }
    *title = file.substr(begin, end - begin);
    return true;
}

static bool ParseContent(const std::string &file, std::string *content)
{
    //去标签，基于一个简易的状态机
    enum status
    {                                                                                                                                                                                 
        LABLE,
        CONTENT 
    };
    enum status s = LABLE;
    for(char c : file)
    {
        switch(s)
        {
            case LABLE:
                if(c == '>') s = CONTENT;
                break;
            case CONTENT:
                if(c == '<') s = LABLE;
                else 
                {
                    // 我们不想保留原始文件中的\n，因为我们想用\n作为html解析之后的文本的分隔符
                    if(c == '\n') c = ' ';
                    content->push_back(c);
                }
                break;
            default:
                break;
        }
    }
    return true;
}

static bool ParseUrl(const std::string &file_path, std::string *url)                                                                                                                  
{    
    std::string url_head = "https://www.boost.org/doc/libs/1_83_0/doc/html";    
    std::string url_tail = file_path.substr(src_path.size());//将data/input截取掉    
    *url = url_head + url_tail;//拼接
    return true;    
}

bool ParseHtml(const std::vector<std::string> &files_list, std::vector<DocInfo_t> *results)
{
    for(const std::string &file : files_list)
    {
        // 1.读取文件，Read()
        std::string result;
        if(!ns_util::FileUtil::ReadFile(file, &result))
        {
            continue;
        }
        // 2.解析指定的文件，提取title
        DocInfo_t doc;
        if(!ParseTitle(result, &doc.title))
        {
            continue;
        }
        // 3.解析指定的文件，提取content
        if(!ParseContent(result, &doc.content))
        {
            continue;
        }
        // 4.解析指定的文件路径，构建url
        if(!ParseUrl(file, &doc.url))
        {
            continue;        
        }
 
        // 到这里，一定是完成了解析任务，当前文档的相关结果都保存在了doc里面
        results->push_back(std::move(doc)); // 本质会发生拷贝，效率肯能会比较低，这里我们使用move后的左值变成了右值，去调用push_back的右值引用版本
    }
    return true;                                                                                                                                                                                                                                                    
}


bool SaveHtml(const std::vector<DocInfo_t> &results, const std::string &output)
{
 
    #define SEP '\3'//分割符---区分标题、内容和网址
 
    // 按照二进制的方式进行写入
    std::ofstream out(output, std::ios::out | std::ios::binary);
    if(!out.is_open()){
        std::cerr << "open " << output << " failed!" << std::endl;
        return false;
    }
 
    // 到这里就可以进行文件内容的写入了
    for(auto &item : results)
    {
        std::string out_string;
        out_string = item.title;//标题
        out_string += SEP;//分割符
        out_string += item.content;//内容
        out_string += SEP;//分割符
        out_string += item.url;//网址
        out_string += '\n';//换行，表示区分每一个文件
 
        out.write(out_string.c_str(), out_string.size());
    }
 
    out.close();
 
    return true;
}

int main()
{
    std::vector<std::string> files_list;
    // 第一步：递归式的把每个html文件名带路径，保存到files_list中，方便后期进行一个一个的文件读取
    if(!EnumFile(src_path, &files_list)) //EnumFile--枚举文件
    {
        std::cerr << "enum file name error! " << std::endl;
        return 1;
    }
 
    // 第二步：按照files_list读取每个文件的内容，并进行解析
    std::vector<DocInfo_t> results;
    if(!ParseHtml(files_list, &results))//ParseHtml--解析html
    {
        std::cerr << "parse html error! " << std::endl;
        return 2;
    }
 
    // 第三步：把解析完毕的各个文件的内容写入到output，按照 \3 作为每个文档的分隔符
    if(!SaveHtml(results, output))//SaveHtml--保存html
    {
        std::cerr << "save html error! " << std::endl;
        return 3;
    }
    return 0;
}


