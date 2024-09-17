#include <opencv2/opencv.hpp>
#include <string>
#include <filesystem>
#include <fstream>
#include <vector>
std::string res_dir_path = "../../train";
namespace fs = std::filesystem;



//todo наша задача - создать сервис, который сможет по путик директорию скопировать её файлы+с путями и поддиректориями и загрузить их в бд
//скорее всего будем использовать dfs проход т.к. мы будем создавать поддиректории для хранения нужных нам файлов

//обратный процесс с восстановлением вещй обранто
//поидее можно заюзать многопоточку
bool has_child_directory(const fs::path &dir)
{
    for (auto itr = fs::directory_iterator(dir); itr != fs::directory_iterator(); ++itr)
    {
        if (itr->is_directory())
        {
            std::string s = itr->path().string();
            if ((s != ".") && (s != ".."))
                return true;
        }
    }
    return false;
}

template <unsigned dir_limit>
void dfFiles(std::string trainDir,std::vector<std::string>&files)
{
    int count=0;
    for (const auto& entry : fs::directory_iterator(trainDir)) {
        count++;
        if(count>dir_limit)
        {
            return;
        }
        if (fs::is_directory(entry.path())) {

            dfFiles<dir_limit>(entry.path().string(),files);
        }
        else
        {
            files.push_back(entry.path().string());
        }
    }

}

std::vector<std::string> dfsHelper(std::string trainDir)
{
    std::vector<std::string> files;
    try {
        auto good=fs::exists(trainDir) && fs::is_directory(trainDir);
        if(!good)
        {
            return files;
        }
    } catch (const fs::filesystem_error& e) {
        return files;
    }
    dfFiles<100>(trainDir,files);
    return files;
}

std::pair<std::vector<std::string>,std::vector<std::string>> getFiles2(std::string trainDir)
{
    std::vector<std::string> files;
    std::vector<std::string> directories;
    try {
        auto good=fs::exists(trainDir) && fs::is_directory(trainDir);
        if(!good)
        {
            return {files,directories};
        }
    } catch (const fs::filesystem_error& e) {
        return {files,directories};
    }

    for (const auto& entry : fs::recursive_directory_iterator(trainDir)) {
        if (fs::is_directory(entry.path())) {

           directories.push_back(fs::canonical(entry.path()).string());
        }
        else
        {
            files.push_back(fs::canonical(entry.path()).string());
        }
    }
    return {files,directories};
}



int main() {
    std::string trainDir = res_dir_path;
    auto dir_path=fs::canonical(res_dir_path);

    auto vector= getFiles2(res_dir_path);
    fs::path pp(vector.first[vector.first.size()-1]);
    //pp=fs::canonical(pp);
    std::ifstream in(vector.first[vector.first.size()-1]);
    std::cout<<absolute(pp)<<'\t'<<fs::relative(vector.first[vector.first.size()-1])<<'\n';

    std::cout<<fs::file_size(pp)<<'\n';
    std::cout<<in.rdbuf();
    in.close();






    // Create a 3-channel image with a size of 400x400
    cv::Mat image = cv::Mat::zeros(400, 400, CV_8UC3);

    // Draw a red circle in the center of the image
    cv::circle(image, cv::Point(200, 200), 100, cv::Scalar(0, 0, 255), -1);

    // Display the image
    cv::imshow("Image", image);

    // Wait for a key press
    cv::waitKey(0);

    // Release resources
    cv::destroyAllWindows();

    return 0;
}

