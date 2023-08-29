#pragma once
#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <boost/algorithm/string.hpp>
#include "cppjieba/Jieba.hpp"//引入头文件（确保你建立的没有错误才可以使用）

namespace ns_util
{
    class FileUtil
    {
    public:
        static bool ReadFile(const std::string &file_path, std::string *out)
        {
            std::ifstream in(file_path, std::ios::in);
            if (!in.is_open())
            {
                std::cerr << "open file " << file_path << " error" << std::endl;
                return false;
            }
            std::string line;
            while (std::getline(in, line)) // 如何理解getline读取到文件结束呢？？getline的返回值是一个&，while(bool), 本质是因为重载了强制类型转化
            {
                *out += line;
            }
            in.close();
            return true;
        }
    };

    class StringUtil
    {
    public:
        // 切分字符串
        static void Splist(const std::string &target, std::vector<std::string> *out, const std::string &sep)
        {
            // boost库中的split函数
            boost::split(*out, target, boost::is_any_of(sep), boost::token_compress_on);
            // 第一个参数：表示你要将切分的字符串放到哪里
            // 第二个参数：表示你要切分的字符串
            // 第三个参数：表示分割符是什么，不管是多个还是一个
            // 第四个参数：它是默认可以不传，即切分的时候不压缩，不压缩就是保留空格
            // 如：字符串为aaaa\3\3bbbb\3\3cccc\3\3d
            // 如果不传第四个参数 结果为aaaa  bbbb  cccc  d
            // 如果传第四个参数为boost::token_compress_on 结果为aaaabbbbccccd
            // 如果传第四个参数为boost::token_compress_off 结果为aaaa  bbbb  cccc  d
        }
    };
    
    //下面这5个是分词时所需要的词库路径
    const char* const DICT_PATH = "./dict/jieba.dict.utf8";    
    const char* const HMM_PATH = "./dict/hmm_model.utf8";    
    const char* const USER_DICT_PATH = "./dict/user.dict.utf8";    
    const char* const IDF_PATH = "./dict/idf.utf8";    
    const char* const STOP_WORD_PATH = "./dict/stop_words.utf8";    
    class JiebaUtil    
    {    
    private:    
        static cppjieba::Jieba jieba; //定义静态的成员变量（需要在类外初始化）   
    public:    
        static void CutString(const std::string &src, std::vector<std::string> *out)    
        {   
            //调用CutForSearch函数，第一个参数就是你要对谁进行分词，第二个参数就是分词后的结果存放到哪里
            jieba.CutForSearch(src, *out);    
        }     
    };    
    //类外初始化，就是将上面的路径传进去，具体和它的构造函数是相关的，具体可以去看一下源代码
    cppjieba::Jieba JiebaUtil::jieba(DICT_PATH, HMM_PATH, USER_DICT_PATH, IDF_PATH, STOP_WORD_PATH);


}
