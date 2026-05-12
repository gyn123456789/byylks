#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

std::string readFileToString(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "无法打开文件: " << filePath << std::endl;
        return "";
    }

    std::ostringstream contentStream;
    contentStream << file.rdbuf();  // 将文件内容读入 stringstream
    file.close();

    return contentStream.str();  // 返回完整字符串
}