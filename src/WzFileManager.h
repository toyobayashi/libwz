#ifndef SRC_WZFILEMANAGER_H_
#define SRC_WZFILEMANAGER_H_
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "wz/Result.h"
#include "wz/Util/Defines.h"
#include "wz/WzEnums.h"

namespace wz {

class WzFile;
class WzImage;
class WzDirectory;
class WzObject;

class WzFileManager final {
 public:
  static WzFileManager* fileManager;
  static constexpr const char* CANVAS_DIRECTORY_NAME = "_Canvas";
  static constexpr const char* BIG_BANG_MARKER =
      "BigBang!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!";
  static constexpr const char* BIG_BANG_2_MARKER =
      "BigBang2!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!";

  ~WzFileManager();
  WzFileManager();
  explicit WzFileManager(const std::string& directory,
                         bool is_standalone_wz_file);

  WZ_DISALLOW_COPY_AND_MOVE(WzFileManager)

  const std::string& WzBaseDirectory() const {
    return init_as_64bit_ ? baseDirWithData_ : baseDir_;
  }
  const std::string& BaseDir() const { return baseDir_; }
  bool IsStandAloneWzFile() const { return is_standalone_wz_file_; }
  bool Is64Bit() const { return init_as_64bit_; }
  bool IsPreBBDataWzFormat() const { return is_pre_bb_data_wz_format_; }

  static bool Detect64BitDirectoryWzFileFormat(
      const std::string& baseDirectoryPath);
  static bool DetectIsPreBBDataWZFileFormat(
      const std::string& baseDirectoryPath);
  static bool DetectIsPreBBDataWZFileFormat(
      const std::string& baseDirectoryPath, WzMapleVersion encryption);
  static bool DetectBetaDataWzFormat(const std::string& baseDirectoryPath);
  static bool DetectBigBang2Format(const std::string& mapleStoryPath,
                                   WzMapleVersion encryption,
                                   bool is64Bit);
  static bool IsBigBangUpdate(WzImage* uiWindow2Image);
  static bool IsBigBang2Update(WzImage* uiWindow2Image);

  void BuildWzFileList();

  Result<WzFile*> LoadWzFile(const std::string& baseName,
                             WzMapleVersion encVersion);
  Result<WzFile*> LoadWzFile(const std::string& baseName,
                             std::unique_ptr<WzFile> wzf);
  bool LoadLegacyDataWzFile(const std::string& baseName,
                            WzMapleVersion encVersion);
  Result<WzImage*> LoadDataWzHotfixFile(const std::string& basePath,
                                        WzMapleVersion encVersion);
  Result<void> LoadCanvasSection(const std::string& canvasFolder,
                                 WzMapleVersion encVersion);
  bool LoadListWzFile(WzMapleVersion fileVersion);

  void UnloadWzFile(WzFile* wzFile);
  void UnloadWzImgFile(WzImage* wzImage);
  void Clear();

  bool IsWzFileLoaded(const std::string& baseName) const;
  std::vector<std::string> GetWzFileNameListFromBase(
      const std::string& baseName) const;
  std::vector<WzDirectory*> GetWzDirectoriesFromBase(
      const std::string& baseName, bool isCanvas = false);
  WzObject* FindWzImageByName(const std::string& baseWzName,
                              const std::string& imageName);
  std::vector<WzObject*> FindWzImagesByName(const std::string& baseWzName,
                                            const std::string& imageName);
  WzDirectory* GetDirectoryByName(const std::string& name) const;

  WzDirectory* operator[](const std::string& name) const {
    return GetDirectoryByName(name);
  }

  static bool ContainsCanvasDirectory(const std::string& path);
  static std::string NormaliseWzCanvasDirectory(
      const std::string& filePathOrBaseFileName);

  static const std::vector<std::string>& ExcludedDirectories();

  const std::unordered_map<std::string, std::unique_ptr<WzFile>>& GetWzFiles()
      const {
    return wzFiles_;
  }
  const std::unordered_map<std::string, std::vector<std::string>>&
  GetWzFilesList() const {
    return wzFilesList_;
  }
  const std::vector<std::string>& GetListWzPaths() const {
    return listWzPaths_;
  }

 private:
  std::string baseDir_;
  std::string baseDirWithData_;
  bool is_standalone_wz_file_ = false;
  bool init_as_64bit_ = false;
  bool is_pre_bb_data_wz_format_ = false;
  bool built_wz_file_directory_list_ = false;

  std::unordered_map<std::string, std::unique_ptr<WzFile>> wzFiles_;
  std::unordered_map<std::string, std::unique_ptr<WzImage>> wzImages_;
  std::unordered_map<std::string, WzDirectory*> wzDirs_;

  std::unordered_map<std::string, std::vector<std::string>> wzFilesList_;
  std::unordered_map<std::string, std::string> wzFilesDirectoryList_;
  std::unordered_set<std::string> wzCanvasSectionLoaded_;
  std::vector<std::string> listWzPaths_;

  std::string GetWzFilePath(const std::string& filePathOrBaseFileName) const;
};

}  // namespace wz
#endif  // SRC_WZFILEMANAGER_H_
