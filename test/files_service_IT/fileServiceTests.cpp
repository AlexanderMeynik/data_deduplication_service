#include <fstream>
#include <filesystem>

#include "testClasses.h"
#include "FileService.h"
static fs::path fix_dir = "../test_data/fixture/";
static fs::path res_dir = "../test_data/res/";

/**
 * @brief This test suite is for directory operations
 * @details The purpose oth these test is to check proper directory mangement
 * @details during it's import/export and other operations
 */
class DirectoryManipulationTests : public ::testing::TestWithParam<fs::path> {
public:
    static void SetUpTestSuite() {
        dbName = "dedup_test_" + std::to_string(2);
        cStr = defaultConfiguration();
        cStr.setDbname(dbName);

        fileService.dbLoad<create>(cStr);
        conn_ = connectIfPossible(cStr).value_or(nullptr);
        ASSERT_TRUE(db_services::checkConnection(conn_));
    }

    static void TearDownTestSuite() {
        fileService.dbDrop(dbName);

        diconnect(conn_);
    }
    void getEntries(std::string_view dirName,std::set<std::tuple<std::string,indexType,indexType ,hash_utils::hash_function>>& loadedFiles,
    std::set<std::string>& loadedDirs);


protected:
    bool checkOrCreateDirectory(std::string_view path)
    {
        if(fs::exists(path)||!fs::is_directory(path))
        {
            fs::create_directories(path);
        }
        return fs::is_directory(path);
    }

    inline static sType fileService;
    inline static std::string dbName;
    inline static myConnString cStr;
    inline static conPtr conn_;
    inline static size_t segmentSize=64;
};

void DirectoryManipulationTests::getEntries(std::string_view dirName, std::set<std::tuple<std::string,indexType,indexType ,hash_utils::hash_function>>& loadedFiles,
                                            std::set<std::string>& loadedDirs)
{
    auto res=fileService.executeInTransaction(&db_services::getEntriesForDirectory, {dirName.data()});
    ASSERT_TRUE(res.has_value());
    for (auto row:res.value()) {
        bool all=std::all_of(row.begin(),row.end(),[](const auto &
        rowelem){return !rowelem.is_null();});
        auto path=fromSpacedPath(row[1].as<std::string>());

        if(all) {
            auto size=row[2].as<indexType>();
            auto segSize=row[3].as<indexType>();
            auto hash1=static_cast<hash_function>(row[6].as<int>());
            loadedFiles.emplace(path,size,segSize,hash1);
        }
        else
        {
            loadedDirs.emplace(path);
        }
    }
}
//todo find a way to swap one file with onther
//todo file/directory not exist returns -1
//todo empty directroy load/delete
//todo temp directory entries(what do we do if we delete all files and

TEST_F(DirectoryManipulationTests, test_created_db_acess) {
    ASSERT_TRUE(fileService.checkConnection());
}
TEST_F(DirectoryManipulationTests, testDirectoryImportDelete) {

    auto hash=SHA_256;
    std::string dirName=getNormalAbs((fix_dir/"block_size"));
    std::string outName=getNormalAbs((res_dir/"block_size"));

    fileService.processDirectory(dirName,segmentSize,hash);
    ASSERT_NO_THROW(fileService.executeInTransaction(&db_services::getEntriesForDirectory, {dirName.c_str()}));

    std::set<std::tuple<std::string,indexType,indexType ,decltype(hash)>> loadedFiles;
    std::set<std::string> loadedDirs;

    DirectoryManipulationTests::getEntries(dirName, loadedFiles, loadedDirs);
    size_t fileSize=0;

    for (auto& entry:fs::recursive_directory_iterator(dirName)) {
        if(fs::is_directory(entry))
        {
            EXPECT_TRUE(loadedDirs.contains(entry.path()));
        }
        else
        {
            auto size=fs::file_size(entry);
            fileSize+=size;
            EXPECT_TRUE(loadedFiles.contains({entry.path(),size,segmentSize,hash}));
        }
        SCOPED_TRACE(entry.path());
    }

    ASSERT_TRUE(checkOrCreateDirectory(outName));
    auto returnR=fileService.loadDirectory(dirName,outName);

    auto compare=file_services::compareDirectories(dirName,outName,segmentSize);

    ASSERT_EQ(compare[0],0);
    ASSERT_EQ(compare[1],0);
    ASSERT_EQ(compare[3],fileSize);
    fileService.deleteDirectory(dirName);

    auto res=fileService.executeInTransaction(&db_services::getEntriesForDirectory, {dirName.c_str()});
    ASSERT_TRUE(res.has_value());
    ASSERT_TRUE(res.value().empty());
}


TEST_F(DirectoryManipulationTests, testDirectoryLoadWithdeleteOption) {

    auto hash=SHA_256;
    std::string dirName=getNormalAbs((fix_dir/"block_size"));
    std::string outName=getNormalAbs((res_dir/"block_size"));

    fileService.processDirectory(dirName,segmentSize,hash);
    ASSERT_NO_THROW(fileService.executeInTransaction(&db_services::getEntriesForDirectory, {dirName.c_str()}));

    std::set<std::tuple<std::string,indexType,indexType ,decltype(hash)>> loadedFiles;
    std::set<std::string> loadedDirs;

    DirectoryManipulationTests::getEntries(dirName, loadedFiles, loadedDirs);
    size_t fileSize=0;

    for (auto& entry:fs::recursive_directory_iterator(dirName)) {
        if(fs::is_directory(entry))
        {
            EXPECT_TRUE(loadedDirs.contains(entry.path()));
        }
        else
        {
            auto size=fs::file_size(entry);
            fileSize+=size;
            EXPECT_TRUE(loadedFiles.contains({entry.path(),size,segmentSize,hash}));
        }
        SCOPED_TRACE(entry.path());
    }

    ASSERT_TRUE(checkOrCreateDirectory(outName));
    auto returnR=fileService.loadDirectory<NoCreateMain,Remove>(dirName,outName);

    auto compare=file_services::compareDirectories(dirName,outName,segmentSize);
    ASSERT_EQ(compare[0],0);
    ASSERT_EQ(compare[1],0);
    ASSERT_EQ(compare[3],fileSize);

    auto res=fileService.executeInTransaction(&db_services::getEntriesForDirectory, {dirName.c_str()});
    ASSERT_TRUE(res.has_value());
    ASSERT_TRUE(res.value().empty());
}



TEST_F(DirectoryManipulationTests, FileServiceDisconnect) {
        fileService.disconnect();
    ASSERT_TRUE(!fileService.checkConnection());

}


