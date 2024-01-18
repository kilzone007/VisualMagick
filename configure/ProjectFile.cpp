/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%  Copyright 2014-2021 ImageMagick Studio LLC, a non-profit organization      %
%  dedicated to making software imaging solutions freely available.           %
%                                                                             %
%  You may not use this file except in compliance with the License.  You may  %
%  obtain a copy of the License at                                            %
%                                                                             %
%    http://www.imagemagick.org/script/license.php                            %
%                                                                             %
%  Unless required by applicable law or agreed to in writing, software        %
%  distributed under the License is distributed on an "AS IS" BASIS,          %
%  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   %
%  See the License for the specific language governing permissions and        %
%  limitations under the License.                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
*/
#include "stdafx.h"
#include "Project.h"
#include "ProjectFile.h"
#include "Shared.h"
#include <algorithm>
#include <map>


static const wstring
  relativePathForConfigure(L"../../"),
  relativePathForProject(L"../../../");

ProjectFile::ProjectFile(const ConfigureWizard *wizard,Project *project,
  const wstring &prefix,const wstring &name)
  : _wizard(wizard),
    _project(project),
    _prefix(prefix),
    _name(name)
{
  initialize(project);
  loadAliases();
}

ProjectFile::ProjectFile(const ConfigureWizard *wizard,Project *project,
  const wstring &prefix,const wstring &name,const wstring &reference)
  : _wizard(wizard),
    _project(project),
    _prefix(prefix),
    _name(name),
    _reference(reference)
{
  initialize(project);
}

vector<wstring> &ProjectFile::dependencies()
{
  return(_dependencies);
}

wstring ProjectFile::fileName() const
{
  return(_fileName);
}

wstring ProjectFile::guid() const
{
  return(_guid);
}

wstring ProjectFile::prefix() const
{
  return(_prefix);
}

wstring ProjectFile::name() const
{
  return(_prefix+L"_"+_name);
}

vector<wstring> &ProjectFile::aliases()
{
  return(_aliases);
}

void ProjectFile::initialize(Project* project)
{
  _minimumVisualStudioVersion=VSEARLIEST;
  setFileName();
  _guid=createGuid();

  foreach(wstring,dep,project->dependencies())
  {
    _dependencies.push_back(*dep);
  }

  foreach(wstring,inc,project->includes())
  {
    _includes.push_back(*inc);
  }

  foreach(wstring,inc,project->definesLib())
  {
    _definesLib.push_back(*inc);
  }
}

bool ProjectFile::isSrcFile(const wstring &fileName)
{
  if (endsWith(fileName,L".asm"))
    return(true);

  return(isValidSrcFile(fileName));
}

bool ProjectFile::isExcluded(const wstring &fileName)
{
  if (contains(_project->excludes(),fileName))
    return true;

  if (contains(_project->platformExcludes(_wizard->platform()),fileName))
    return true;

  if (endsWith(fileName,L".h"))
    return isExcluded(replace(fileName,L".h",L".c")) || isExcluded(replace(fileName,L".h",L".cc"));

  return false;
}

void ProjectFile::loadAliases()
{
  wifstream
    aliases;

  wstring
    fileName,
    line;

  if (!_project->isExe() || !_project->isModule())
    return;

  fileName=L"..\\" + _project->name() + L"\\Aliases." + _name + L".txt";

  aliases.open(fileName);
  if (!aliases)
    return;

  while (!aliases.eof())
  {
    line=readLine(aliases);
    if (!line.empty())
      _aliases.push_back(line);
  }

  aliases.close();
}

bool ProjectFile::isSupported(const VisualStudioVersion visualStudioVersion) const
{
  return(visualStudioVersion >= _minimumVisualStudioVersion);
}

void ProjectFile::loadConfig()
{
  wifstream
    config;

  wstring
    fileName,
    line;

  if (!_project->isModule())
    return;

  fileName=L"..\\" + _project->name() + L"\\Config." + _name + L".txt";

  config.open(fileName);
  if (!config)
    return;

  while (!config.eof())
  {
    line=readLine(config);
    if (line == L"[DEPENDENCIES]")
      addLines(config,_dependencies);
    else if (line == L"[INCLUDES]")
      addLines(config,_includes);
    else if (line == L"[CPP]")
      addLines(config,_cppFiles);
    else if (line == L"[VISUAL_STUDIO]")
      _minimumVisualStudioVersion=parseVisualStudioVersion(readLine(config));
    else if (line == L"[DEFINES_LIB]")
      addLines(config,_definesLib);
  }

  config.close();
}

void ProjectFile::merge(ProjectFile *projectFile)
{
  merge(projectFile->_dependencies,_dependencies);
  merge(projectFile->_includes,_includes);
  merge(projectFile->_cppFiles,_cppFiles);
  merge(projectFile->_definesLib,_definesLib);
}

void ProjectFile::write(const vector<Project*> &allprojects)
{
  wofstream
    file;

  wstring
    projectDir(L"..\\VisualStudioProjects\\" + name());

  CreateDirectoryW(projectDir.c_str(), NULL);

  file.open(projectDir + L"\\" + _fileName);
  if (!file)
    return;

  loadSource();

  write(file,allprojects);

  if (_project->isExe() && _project->icon() != L"")
  {
    file.close();

    file.open(projectDir + L"\\" + name() + L".rc");
    if (!file)
      return;

    file << "#define IDI_ICON1 101" << endl;
    file << "IDI_ICON1 ICON \"" << relativePathForProject <<  _project->icon() << "\"" << endl;
  }

  file.close();
}

bool ProjectFile::isLib() const
{
  return(_project->isLib() || (_wizard->solutionType() != SolutionType::DYNAMIC_MT && _project->isDll()));
}

wstring ProjectFile::outputDirectory() const
{
  if (_project->isFuzz())
    return(_wizard->fuzzBinDirectory());

  if (isLib())
    return(_wizard->libDirectory());

  return(_wizard->binDirectory());
}

void ProjectFile::addFile(const wstring &directory, const wstring &name)
{
  wstring
    header_file,
    src_file;

  foreach_const(wstring,ext,validSrcFiles)
  {
    src_file=directory + L"/" + name + *ext;

    if (PathFileExists((relativePathForConfigure + src_file).c_str()))
    {
      _srcFiles.push_back(relativePathForProject + src_file);

      header_file=directory + L"/" + name + L".h";
      if (PathFileExists((relativePathForConfigure + header_file).c_str()))
        _includeFiles.push_back(relativePathForProject + header_file);

      break;
    }
  }

  if (!_project->isExe())
    return;

  foreach_const(wstring,ext,validSrcFiles)
  {
    src_file=directory + L"/main" + *ext;

    if (PathFileExists((relativePathForConfigure + src_file).c_str()))
    {
      _srcFiles.push_back(relativePathForProject + src_file);

      header_file=directory + L"/" + name + L".h";
      if (PathFileExists((relativePathForConfigure + header_file).c_str()))
        _includeFiles.push_back(relativePathForProject + header_file);

      break;
    }
  }
}

void ProjectFile::addLines(wifstream &config,vector<wstring> &container)
{
  wstring
    line;

  while (!config.eof())
  {
    line=readLine(config);
    if (line.empty())
      return;

    std::replace(line.begin(), line.end(), L'\\', L'/');

    if (!contains(container,line))
      container.push_back(line);
  }
}

wstring ProjectFile::asmOptions()
{
  switch (_wizard->platform())
  {
    case Platform::X86: return(L"ml /nologo /c /Cx /safeseh /coff /Fo\"$(IntDir)%(Filename).obj\" \"%(FullPath)\"");
    case Platform::X64: return(L"ml64 /nologo /c /Cx /Fo\"$(IntDir)%(Filename).obj\" \"%(FullPath)\"");
    case Platform::ARM64: return(L"armasm64 \"%(FullPath)\" -o \"$(IntDir)%(Filename).obj\"");
    default: throw;
  }
}

wstring ProjectFile::getFilter(const wstring &fileName,vector<wstring> &filters)
{
  wstring
    filter,
    folder;

  filter=replace(fileName,L"..\\",L"");
  if (filter.find_first_of(L"\\") == filter.find_last_of(L"\\"))
    return L"";

  filter=filter.substr(filter.find_first_of(L"\\") + 1);

  folder=filter;
  while (folder.find_first_of(L"\\") != -1)
  {
    folder=folder.substr(0, folder.find_last_of(L"\\"));
    if (!contains(filters,folder))
      filters.push_back(folder);
  }

  filter=filter.substr(0, filter.find_last_of(L"\\"));

  return filter;
}

wstring ProjectFile::getIntermediateDirectoryName(const bool debug)
{
  wstring
    directoryName;

  directoryName = (debug ? L"Debug\\" : L"Release\\");
  directoryName += _wizard->solutionName() + L"-" + _wizard->platformName() + L"\\";
  directoryName += _prefix + L"_" + _name + L"\\";
  return(directoryName);
}

wstring ProjectFile::getTargetName(const bool debug)
{
  wstring
    targetName;

  targetName=_prefix+L"_";
  if (_prefix.compare(L"FILTER") != 0)
    targetName+=wstring(debug ? L"DB" : L"RL")+L"_";
  targetName+=_name+L"_";
  return(targetName);
}

void ProjectFile::loadModule(const wstring &directory)
{
  if (!_reference.empty())
    addFile(directory, _reference);
  else
    addFile(directory, _name);
}

void ProjectFile::loadSource()
{
  wstring
    resourceFile;

  foreach (wstring,dir,_project->directories())
  {
    if ((_project->isModule()) && (_project->isExe() || (_project->isDll() && _wizard->solutionType() == SolutionType::DYNAMIC_MT)))
      loadModule(*dir);
    else
      loadSource(*dir);
  }

  resourceFile=relativePathForProject + _project->name() + L"\\ImageMagick\\ImageMagick.rc";
  if (PathFileExists(resourceFile.c_str()))
    _resourceFiles.push_back(resourceFile);

  /* This resource file is used by the ImageMagick projects */
  resourceFile=relativePathForProject + _project->name() + L"\\ImageMagick.rc";
  if (PathFileExists(resourceFile.c_str()))
    _resourceFiles.push_back(resourceFile);
}

void ProjectFile::loadSource(const wstring &directory)
{
  HANDLE
    fileHandle;

  WIN32_FIND_DATA
    data;

  if (contains(_project->platformExcludes(_wizard->platform()),directory))
    return;

  fileHandle=FindFirstFile((relativePathForConfigure + directory + L"\\*.*").c_str(),&data);
  do
  {
    if (fileHandle == INVALID_HANDLE_VALUE)
      return;

    if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
      continue;

    if (isExcluded(data.cFileName))
      continue;

    if (isSrcFile(data.cFileName))
      _srcFiles.push_back(relativePathForProject + directory + L"/" + data.cFileName);
    else if (endsWith(data.cFileName,L".h"))
      _includeFiles.push_back(relativePathForProject + directory + L"/" + data.cFileName);
    else if (endsWith(data.cFileName,L".rc"))
      _resourceFiles.push_back(relativePathForProject + directory + L"/" + data.cFileName);

  } while (FindNextFile(fileHandle,&data));

  FindClose(fileHandle);
}

wstring ProjectFile::nasmOptions(const wstring &folder)
{
  wstring
    result=L"";

  if (_wizard->platform() == Platform::ARM64)
    return(result);

  result += L"..\\build\\nasm -i\"" + folder +L"\"";

  if (_wizard->platform() == Platform::X86)
    result += L" -fwin32 -DWIN32";
  else
    result += L" -fwin64 -DWIN64 -D__x86_64__";

  foreach_const(wstring,include,_project->includesNasm())
    result += L" -i\"" + relativePathForProject + *include + L"\"";

  result += L" -o \"$(IntDir)%(Filename).obj\" \"%(FullPath)\"";
  return(result);
}

void ProjectFile::merge(vector<wstring> &input, vector<wstring> &output)
{
  foreach (wstring,value,input)
  {
    if (!contains(output,*value))
      output.push_back(*value);
  }
}

void ProjectFile::setFileName()
{
  _fileName = L"CMakeLists.txt";
}

wstring ProjectFile::createGuid()
{
  GUID
    guid;

  RPC_WSTR
    guidStr;

  wstring
    result;

  (void) CoCreateGuid(&guid);
  (void) UuidToString(&guid,&guidStr);
  result=wstring((wchar_t *) guidStr);
  transform(result.begin(),result.end(),result.begin(),::toupper);
  RpcStringFree(&guidStr);
  return result;
}

void ProjectFile::write(wofstream &file,const vector<Project*> &allProjects)
{
  writeHeader(file);

  writeTarget(file);

  writeIncludeDirectories(file);

  writeCompileDefinitions(file);

  writeCompileOptions(file);

  writeProperties(file);

  writeFiles(file,_srcFiles);
  writeFiles(file,_includeFiles);
  writeFiles(file,_resourceFiles);
  writeIcon(file);

  writeProjectReferences(file,allProjects);
}

void ProjectFile::writeHeader(wofstream& file)
{
  file << "cmake_minimum_required(VERSION " << _wizard->cmakeMinVersion() << ")" << endl;
  file << "project(" << name() << " LANGUAGES C CXX ASM" << ")" << endl;

  file << "set(CMAKE_CXX_STANDARD 17)" << endl;
  file << "set(CMAKE_CXX_STANDARD_REQUIRED ON)" << endl;

  file << "set(CMAKE_C_STANDARD 17)" << endl;
  file << "set(CMAKE_C_STANDARD_REQUIRED ON)" << endl;

  if (_project->useUnicode())
  {
    file << "add_definitions(-UNICODE -D_UNICODE)" << endl;
  }
}

void ProjectFile::writeTarget(wofstream& file)
{
  if (isLib())
  {
    file << "add_library(" << name() << " STATIC" << ")" << endl;
  }
  else if (_project->isDll())
  {
    file << "add_library(" << name() << " SHARED" << ")" << endl;
  }
  else if (_project->isExe())
  {
    file << "add_executable(" << name() << ")" << endl;
  }
}

void ProjectFile::writeIncludeDirectories(wostream& file)
{
  file << "target_include_directories(" << name() << " PUBLIC ";
  for (wstring projectDir : _project->directories())
  {
    bool skip = false;

    for (wstring includeDir : _includes)
    {
      if (projectDir.find(includeDir) == 0)
      {
        skip = true;
        break;
      }
    }

    if (!skip)
    {
      file << "\n" << "  " << relativePathForProject << projectDir;
    }
  }

  for (wstring includeDir : _includes)
  {
    file << "\n" << "  " << relativePathForProject << includeDir;
  }

  if (_wizard->useOpenCL())
  {
    file << "\n" << "  " << relativePathForProject << L"VisualMagick/OpenCL";
  }
  file << "\n)" << endl;
}

void ProjectFile::writeCompileDefinitions(wostream& file)
{
  file << "target_compile_definitions(" << name() << " PRIVATE ";
  file << "\n" << "  $<$<CONFIG:Debug>:_DEBUG>";
  file << "\n" << "  $<$<CONFIG:Release>:NDEBUG>";
  file << "\n" << "  _WINDOWS";
  file << "\n" << "  WIN32";
  file << "\n" << "  _VISUALC_";
  file << "\n" << "  NeedFunctionPrototypes";

  for (wstring def : _project->defines())
  {
    file << "\n" << "  " << def;
  }

  if (isLib() || (_wizard->solutionType() != SolutionType::DYNAMIC_MT && (_project->isExe())))
  {
    for (wstring def : _definesLib)
    {
      file << "\n" << "  " << def;
    }
    file << "\n" << "  _LIB";
  }
  else if (_project->isDll())
  {
    for (wstring def : _project->definesDll())
    {
      file << "\n" << "  " << def;
    }
    file << "\n" << "  _DLL";
    file << "\n" << "  _MAGICKMOD_";
  }

  if (_project->isExe() && _wizard->solutionType() != SolutionType::STATIC_MT)
  {
    file << "\n" << "  _AFXDLL";
  }
  if (_wizard->includeIncompatibleLicense())
  {
    file << "\n" << "  _MAGICK_INCOMPATIBLE_LICENSES_";
  }
  file << "\n)" << endl;
}

void ProjectFile::writeCompileOptions(wostream& file)
{
  file << "target_compile_options(" << name() << " PRIVATE /W" << _project->warningLevel();
  if (_project->treatWarningAsError())
  {
    file << " /WX";
  }
  if (_project->compiler(_wizard->visualStudioVersion()) == Compiler::CPP)
  {
    file << " /TP";
  }
  file << " /Zi";
  if (_wizard->useOpenMP())
  {
    file << " /openmp";
  }
  file << " /FC";
  file << " /source-charset:utf-8";
  file << ")" << endl;
}

void ProjectFile::writeProperties(wostream& file)
{
  if (_project->isFuzz())
  {
    file << "set_target_properties(" << name() << " PROPERTIES " << endl;
    file << "  RUNTIME_OUTPUT_DIRECTORY " << _wizard->fuzzBinDirectory() << endl;
    file << ")" << endl;
  }
}

void ProjectFile::writeAdditionalDependencies(wofstream &file,const wstring &separator)
{
  foreach (wstring,lib,_project->libraries())
  {
    file << separator << *lib;
  }
}

void ProjectFile::writeAdditionalIncludeDirectories(wofstream &file,const wstring &separator)
{
  foreach (wstring,projectDir,_project->directories())
  {
    bool
      skip;

    skip=false;
    foreach (wstring,includeDir,_includes)
    {
      if ((*projectDir).find(*includeDir) == 0)
      {
        skip=true;
        break;
      }
    }

    if (!skip)
      file << separator << relativePathForProject <<  *projectDir;
  }
  foreach (wstring,includeDir,_includes)
  {
    file << separator << relativePathForProject << *includeDir;
  }
  if (_wizard->useOpenCL())
    file << separator << relativePathForProject << L"VisualMagick/OpenCL";
}

void ProjectFile::writeIcon(wofstream &file)
{
  if (!_project->isExe() || _project->icon() == L"")
    return;

  file << "target_sources(" << name() << " PRIVATE" << endl;
  file << "  " << name() << ".rc" << endl;
  file << ")" << endl;
}

void ProjectFile::writeFiles(wofstream &file,const vector<wstring> &collection)
{
  int
    count;

  map<wstring, int>
    fileCount;

  wstring
    fileName,
    folder,
    name;

  if (collection.size() == 0)
    return;

  file << "target_sources(" << this->name() << " PRIVATE" << endl;
  for (const wstring& f : collection)
  {
    file << "  " << f << endl;
  }
  file << ")" << endl;

  for (const wstring& f : collection)
  {
    if (!endsWith(f, L".asm") && !endsWith(f, L".rc") && !endsWith(f, L".h"))
    {
      if (_project->compiler(_wizard->visualStudioVersion()) == Compiler::CPP)
      {
        file << "set_source_files_properties(" << f << " PROPERTIES LANGUAGE CXX)" << endl;
      }
    }
  }
}

void ProjectFile::writePreprocessorDefinitions(wofstream &file,const bool debug)
{
  file << (debug ? "_DEBUG" : "NDEBUG") << ";_WINDOWS;WIN32;_VISUALC_;NeedFunctionPrototypes";
  foreach (wstring,def,_project->defines())
  {
    file << ";" << *def;
  }
  if (isLib() || (_wizard->solutionType() != SolutionType::DYNAMIC_MT && (_project->isExe())))
  {
    foreach (wstring,def,_definesLib)
    {
      file << ";" << *def;
    }
    file << ";_LIB";
  }
  else if (_project->isDll())
  {
    foreach (wstring,def,_project->definesDll())
    {
      file << ";" << *def;
    }
    file << ";_DLL;_MAGICKMOD_";
  }
  if (_project->isExe() && _wizard->solutionType() != SolutionType::STATIC_MT)
    file << ";_AFXDLL";
  if (_wizard->includeIncompatibleLicense())
    file << ";_MAGICK_INCOMPATIBLE_LICENSES_";
}

void ProjectFile::writeProjectReferences(wofstream &file,const vector<Project*> &allProjects)
{
  size_t
    index;

  wstring
    projectName,
    projectFileName;

  bool hasDep = false;

  for (const wstring& dep : _dependencies)
  {
    projectName = dep;
    projectFileName = L"";
    index = dep.find(L">");
    if (index != -1)
    {
      projectName = dep.substr(0, index);
      projectFileName = dep.substr(index + 1);
    }

    for (const auto& depp : allProjects)
    {
      if (depp->name() != projectName)
        continue;

      for (const auto& deppf : depp->files())
      {
        if (projectFileName != L"" && deppf->_name != projectFileName)
          continue;

        if (!hasDep)
        {
          hasDep = true;
          file << "target_link_libraries(" << name() << " PUBLIC " << endl;
        }

        file << "  " << deppf->name() << endl;
      }
    }
  }

  if (hasDep)
  {
    file << ")" << endl;
  }
}
