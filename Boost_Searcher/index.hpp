#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <unordered_map>
#include <mutex>
#include "util.hpp"

namespace ns_index
{
    struct DocInfo // 文档信息节点
    {
        std::string title;   // 文档的标题
        std::string content; // 文档对应的去标签后的内容
        std::string url;     // 官网文档的url
        uint64_t doc_id;     // 文档的ID
    };

    struct InvertedElem // 倒排对应的节点
    {
        uint64_t doc_id;  // 文档ID
        std::string word; // 关键字（通过关键字可以找到对应的ID）
        int weight;       // 权重---根据权重对文档进行排序展示
    };

    typedef std::vector<InvertedElem> InvertedList;

    class Index
    {
    private:
        // 正排索引的数据结构采用数组，数组下标就是天然的文档ID
        std::vector<DocInfo> forward_index; // 正排索引

        // 倒排索引一定是一个关键字和一组（个）InvertedElem对应[关键字和倒排拉链的映射关系]
        std::unordered_map<std::string, InvertedList> inverted_index;

    private:
        Index() {} // 这个一定要有函数体，不能delete
        Index(const Index &) = delete;
        Index &operator=(const Index &) = delete;
        static Index *instance;
        static std::mutex mtx; // C++互斥锁，防止多线程获取单例存在的线程安全问题

    public:
        ~Index() {}

    public:
        // 获取index单例
        static Index *GetInstance()
        {
            if (nullptr == instance) // 双重判定空指针, 降低锁冲突的概率, 提高性能
            {
                mtx.lock(); // 加锁
                if (nullptr == instance)
                {
                    instance = new Index(); // 获取单例
                }
                mtx.unlock(); // 解锁
            }
            return instance;
        }

        // 根据doc_id找到正排索引对应doc_id的文档内容
        DocInfo *GetForwardIndex(uint64_t doc_id)
        {
            // 如果这个doc_id已经大于正排索引的元素个数，则索引失败
            if (doc_id >= forward_index.size())
            {
                std::cout << "doc_id out range, error!" << std::endl;
                return nullptr;
            }
            return &forward_index[doc_id]; // 否则返回相应doc_id的文档内容
        }

        // 根据倒排索引的关键字word，获得倒排拉链
        InvertedList *GetInvertedList(const std::string &word)
        {
            auto iter = inverted_index.find(word);
            if (iter == inverted_index.end())
            {
                std::cerr << " have no InvertedList" << std::endl;
                return nullptr;
            }
            return &(iter->second);
        }

        // 根据去标签，格式化后的文档，构建正排和倒排索引
        // 将数据源的路径：data/raw_html/raw.txt传给input即可，这个函数用来构建索引
        bool BuildIndex(const std::string &input)
        {
            // 在上面SaveHtml函数中，我们是以二进制的方式进行保存的，那么读取的时候也要按照二进制的方式读取，读取失败给出提示
            std::ifstream in(input, std::ios::in | std::ios::binary);
            if (!in.is_open())
            {
                std::cerr << "sory, " << input << " open error" << std::endl;
                return false;
            }

            std::string line;
            int count = 0; // 调试1
            while (std::getline(in, line))
            {
                DocInfo *doc = BuildForwardIndex(line); // 构建正排索引

                if (nullptr == doc)
                {
                    std::cerr << "build " << line << " error" << std::endl;
                    continue;
                }

                BuildInvertedIndex(*doc); // 有了正排索引才能构建倒排索引

                // 调试2
                count++;
                if (count % 50 == 0)
                {
                    std::cout << "当前已经建立的索引文档: " << count << std::endl;
                }
                
            }
            return true;
        }

    private:
        // 构建正排索引
        DocInfo *BuildForwardIndex(const std::string &line)
        {
            // 1.解析line，字符串切分
            // 将line中的内容且分为3段：原始为title\3content\3url\3
            // 切分后：title content url
            std::vector<std::string> results;
            std::string sep = "\3";                           // 行内分隔符
            ns_util::StringUtil::Splist(line, &results, sep); // 字符串切分
            if (results.size() != 3)
            {
                return nullptr;
            }

            // 2.字符串进行填充到DocInfo
            DocInfo doc;
            doc.title = results[0];
            doc.content = results[1];
            doc.url = results[2];
            doc.doc_id = forward_index.size(); // 先进行保存id，在插入，对应的id就是当前doc在vector中的下标

            // 3.插入到正排索引的vector
            forward_index.push_back(std::move(doc)); // 使用move可以减少拷贝带来的效率降低
            return &forward_index.back();
        }

        // 构建倒排索引
        bool BuildInvertedIndex(const DocInfo &doc)
        {
            // 词频统计结构体
            struct word_cnt
            {
                int title_cnt;
                int content_cnt;
                word_cnt() : title_cnt(0), content_cnt(0) {}
            };
            std::unordered_map<std::string, word_cnt> word_map; // 用来暂存词频的映射表

            // 对标题进行分词
            std::vector<std::string> title_words;
            ns_util::JiebaUtil::CutString(doc.title, &title_words);

            // 对标题进行词频统计
            for (auto s : title_words)
            {
                boost::to_lower(s);      // 将我们的分词进行统一转化成为小写的
                word_map[s].title_cnt++; // 如果存在就获取，不存在就新建
            }

            // 对文档内容进行分词
            std::vector<std::string> content_words;
            ns_util::JiebaUtil::CutString(doc.content, &content_words);

            // 对文档内容进行词频统计
            for (auto s : content_words)
            {
                boost::to_lower(s); // 将我们的分词进行统一转化成为小写的
                word_map[s].content_cnt++;
            }

#define X 10
#define Y 1
            // 最终构建倒排
            for (auto &word_pair : word_map)
            {
                InvertedElem item;
                item.doc_id = doc.doc_id; // 倒排索引的id即文档id
                item.word = word_pair.first;
                item.weight = X * word_pair.second.title_cnt + Y * word_pair.second.content_cnt;
                InvertedList &inverted_list = inverted_index[word_pair.first];
                inverted_list.push_back(std::move(item));
            }
            return true;
        }
    };
    Index *Index::instance = nullptr;
    std::mutex Index::mtx;
}
