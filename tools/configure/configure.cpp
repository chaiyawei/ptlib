/*
 * configure.cxx
 *
 * Build options generated by the configure script.
 *
 * Portable Windows Library
 *
 * Copyright (c) 2003 Equivalence Pty. Ltd.
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is Portable Windows Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 *
 * $Log: configure.cpp,v $
 * Revision 1.31  2006/07/06 01:10:14  csoutheren
 * Added parsing of version.h to configure
 * PWLib version variables now set in ptbuildopts.h
 *
 * Revision 1.30  2006/07/05 13:38:17  shorne
 * Fix compile error on MSVC6
 *
 * Revision 1.29  2006/07/04 00:40:16  csoutheren
 * Added capability for multiple feature dependencies
 *
 * Revision 1.28  2006/06/29 02:37:23  csoutheren
 * Fixed compile problem on VC 2005
 *
 * Revision 1.27  2006/06/26 01:02:55  csoutheren
 * Improved configure.exe to ignore case when matching exclude dirs
 * Allow exclusion of drive root directories
 *
 * Revision 1.26  2005/11/30 12:47:42  csoutheren
 * Removed tabs, reformatted some code, and changed tags for Doxygen
 *
 * Revision 1.25  2005/08/15 09:40:58  rjongbloed
 * Captalised the word "disabled" so more obvious.
 *
 * Revision 1.24  2005/08/13 19:13:49  shorne
 * Fix so when feature not found it is marked as disabled.
 *
 * Revision 1.23  2004/12/09 02:05:52  csoutheren
 * Added IF_FEATURE option to allow features dependent on existence/non-existence of
 *  other features
 *
 * Revision 1.22  2004/12/01 11:59:19  csoutheren
 * Incremented version number
 *
 * Revision 1.21  2004/12/01 11:57:03  csoutheren
 * Fixed problem with not finding MSWIN macros with leading spaces and added ability to enable/disable features using --name as as well as --enable-name
 * Thank to Guilhem Tardy
 *
 * Revision 1.20  2004/08/13 01:08:09  csoutheren
 * Changed to look for configure.ac, then configure.in
 *
 * Revision 1.19  2004/07/12 02:32:58  csoutheren
 * Fixed problem when more than two elements in env var
 *
 * Revision 1.18  2004/04/29 02:02:25  csoutheren
 * Removed debugging (oops)
 *
 * Revision 1.17  2004/04/29 02:00:49  csoutheren
 * Fixed problem with checking for NULL error return from FindFirstFile rather than INVALID_HANDLE_VALUE
 *
 * Revision 1.16  2004/04/04 01:30:37  csoutheren
 * Added ability to specify exclude environment variable on configure command line which allows easy switching between MSVC and VS.net 2003
 *
 * Revision 1.15  2004/03/23 06:32:01  csoutheren
 * Fixed problems with multiple directories in exclude spec
 *
 * Revision 1.14  2004/03/16 01:45:17  rjongbloed
 * Fixed locating lbrary in pre-defined search directories.
 * Added version number to configure program.
 * Tidied the --help display.
 *
 * Revision 1.13  2004/03/13 02:50:56  rjongbloed
 * Fixed anomalous message where even though a feature was disabled, a "Located "
 *   directiory message is still displayed causing confusion.
 * Added --disable-<feature> as synonym to existing --no-<feature> to be compatible
 *   with autoconf.
 * Added default value to defines of 1 rather than blank.
 *
 * Revision 1.12  2004/01/30 02:33:58  csoutheren
 * More fixups
 *
 * Revision 1.11  2004/01/30 01:43:41  csoutheren
 * Added excludedir options and environment variable
 *
 * Revision 1.10  2003/11/25 08:21:37  rjongbloed
 * Fixed display of configured items
 *
 * Revision 1.9  2003/11/06 09:13:20  rjongbloed
 * Improved the Windows configure system to allow multiple defines based on file existence. Needed for SDL support of two different distros.
 *
 * Revision 1.8  2003/10/30 01:17:15  dereksmithies
 * Add fix from Jose Luis Urien. Many thanks.
 *
 * Revision 1.7  2003/10/23 21:49:51  dereksmithies
 * Add very sensible fix to limit extent of search. Thanks Ben Lear.
 *
 * Revision 1.6  2003/08/04 05:13:17  dereksmithies
 * Reinforce the disablement if the command lines specifies --no-XXXX to a feature.
 *
 * Revision 1.5  2003/08/04 05:07:08  dereksmithies
 * Command line option now disables feature when feature found on disk.
 *
 * Revision 1.4  2003/05/16 02:03:07  rjongbloed
 * Fixed being able to manually disable a "feature" when does a full disk search.
 *
 * Revision 1.3  2003/05/05 08:39:52  robertj
 * Added ability to explicitly disable a feature, or tell configure exactly
 *   where features library is so it does not need to search for it.
 *
 * Revision 1.2  2003/04/17 03:32:06  robertj
 * Improved windows configure program to use lines out of configure.in
 *
 * Revision 1.1  2003/04/16 08:00:19  robertj
 * Windoes psuedo autoconf support
 *
 */

#pragma warning(disable:4786)
#pragma warning(disable:4996)

#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <list>
#include <algorithm>
#include <stdlib.h>
#include <windows.h>
#include <vector>
#include <map>

#define VERSION "1.7"

static char * VersionTags[] = { "MAJOR_VERSION", "MINOR_VERSION", "BUILD_NUMBER", "BUILD_TYPE" };

using namespace std;

string ToLower(const string & _str)
{
  string str = _str;
  string::iterator r;
  for(r = str.begin(); r != str.end(); ++r)
    *r = tolower(*r);
  return str;
}

class Feature
{
  public:
    Feature() : enabled(true) { }
    Feature(const string & featureName, const string & optionName, const string & optionValue);

    void Parse(const string & optionName, const string & optionValue);
    void Adjust(string & line);
    bool Locate(const char * testDir);

    string featureName;
    string displayName;
    string directorySymbol;
    string simpleDefineName;
    string simpleDefineValue;
		map<string, string> defines;
		map<string, string> defineValues;

    struct CheckFileInfo {
      CheckFileInfo() : found(false), defineName("1") { }

      bool Locate(const string & testDir);

      bool   found;
      string fileName;
      string fileText;
      string defineName;
      string defineValue;
    };
    list<CheckFileInfo> checkFiles;
    list<string> checkDirectories;
    list<string> ifFeature;
    list<string> ifNotFeature;

    string directory;
    bool   enabled;
};


vector<Feature> features;
list<string> excludeDirList;

///////////////////////////////////////////////////////////////////////

Feature::Feature(const string & featureNameParam,
                 const string & optionName,
                 const string & optionValue)
  : featureName(featureNameParam),
    enabled(true)
{
  Parse(optionName, optionValue);
}


void Feature::Parse(const string & optionName, const string & optionValue)
{
  if (optionName == "DISPLAY")
    displayName = optionValue;

  else if (optionName == "DEFINE") {
    int equal = optionValue.find('=');
    if (equal == string::npos)
      simpleDefineName = optionValue;
    else {
      simpleDefineName.assign(optionValue, 0, equal);
      simpleDefineValue.assign(optionValue, equal+1, INT_MAX);
    }
  }

  else if (optionName == "VERSION") {
    int equal = optionValue.find('=');
    if (equal != string::npos)
			defines.insert(pair<string,string>(optionValue.substr(0, equal), optionValue.substr(equal+1)));
  }

  else if (optionName == "CHECK_FILE") {
    int comma = optionValue.find(',');
    if (comma == string::npos)
      return;

    CheckFileInfo check;
    int pipe = optionValue.find('|');
    if (pipe < 0 || pipe > comma)
      check.fileName.assign(optionValue, 0, comma);
    else {
      check.fileName.assign(optionValue, 0, pipe);
      check.fileText.assign(optionValue, pipe+1, comma-pipe-1);
    }

    int equal = optionValue.find('=', comma);
    if (equal == string::npos)
      check.defineName.assign(optionValue, comma+1, INT_MAX);
    else {
      check.defineName.assign(optionValue, comma+1, equal-comma-1);
      check.defineValue.assign(optionValue, equal+1, INT_MAX);
    }

    checkFiles.push_back(check);
  }

  else if (optionName == "DIR_SYMBOL")
    directorySymbol = '@' + optionValue + '@';

  else if (optionName == "CHECK_DIR")
    checkDirectories.push_back(optionValue);

  else if (optionName == "IF_FEATURE") {
    const char * delimiters = ",";
    string::size_type lastPos = optionValue.find_first_not_of(delimiters, 0);
    string::size_type pos = optionValue.find_first_of(delimiters, lastPos);
    while (string::npos != pos || string::npos != lastPos) {
      string str = optionValue.substr(lastPos, pos - lastPos);
      if (str[0] == '!')
        ifNotFeature.push_back(str.substr(1));
      else
        ifFeature.push_back(str);
      lastPos = optionValue.find_first_not_of(delimiters, pos);
      pos = optionValue.find_first_of(delimiters, lastPos);
    }
  }
}


static bool CompareName(const string & line, const string & name)
{
  int pos = line.find(name);
  if (pos == string::npos)
    return false;

  pos += name.length();
  return !isalnum(line[pos]) && line[pos] != '_';
}

void Feature::Adjust(string & line)
{
  if (enabled && line.find("#undef") != string::npos) {
    if (!simpleDefineName.empty() && CompareName(line, simpleDefineName)) {
      line = "#define " + simpleDefineName + ' ';
      if (simpleDefineValue.empty())
        line += '1';
      else
        line += simpleDefineValue;
    }

		map<string,string>::iterator r, s;
		for (r = defines.begin(); r != defines.end(); ++r) {
	    if (CompareName(line, r->first)) {
				s = defineValues.find(r->second);
				if (s != defineValues.end())
					line = "#define " + r->first + ' ' + s->second;
			}
		}

    for (list<CheckFileInfo>::iterator file = checkFiles.begin(); file != checkFiles.end(); file++) {
      if (file->found && CompareName(line, file->defineName)) {
        line = "#define " + file->defineName + ' ' + file->defineValue;
        break;
      }
    }
  }

  if (directorySymbol[0] != '\0') {
    int pos = line.find(directorySymbol);
    if (pos != string::npos)
      line.replace(pos, directorySymbol.length(), directory);
  }
}


bool Feature::Locate(const char * testDir)
{
  if (!enabled)
    return true;

  if (!directory.empty())
    return true;

  if (checkFiles.empty())
    return true;

  string testDirectory = testDir;
  if (testDirectory[testDirectory.length()-1] != '\\')
    testDirectory += '\\';

  list<CheckFileInfo>::iterator file = checkFiles.begin();
  if (!file->Locate(testDirectory)) 
    return false;
  
  while (++file != checkFiles.end())
    file->Locate(testDirectory);

  char buf[_MAX_PATH];
  _fullpath(buf, testDirectory.c_str(), _MAX_PATH);
  directory = buf;

  cout << "Located " << displayName << " at " << directory << endl;

  int pos;
  while ((pos = directory.find('\\')) != string::npos)
    directory[pos] = '/';
  pos = directory.length()-1;
  if (directory[pos] == '/')
    directory.erase(pos);

  return true;
}


bool Feature::CheckFileInfo::Locate(const string & testDirectory)
{
  string filename = testDirectory + fileName;
  ifstream file(filename.c_str(), ios::in);
  if (!file.is_open())
    return false;

  if (fileText.empty())
    found = true;
  else {
    while (file.good()) {
      string line;
      getline(file, line);
      if (line.find(fileText) != string::npos) {
        found = true;
        break;
      }
    }
  }

  return found;
}


bool TreeWalk(const string & _directory)
{
	string directory = ToLower(_directory);

  bool foundAll = false;

  list<string>::const_iterator r = find(excludeDirList.begin(), excludeDirList.end(), directory);
  if (r != excludeDirList.end())
	return false;

  string wildcard = directory;
  wildcard += "*.*";

  WIN32_FIND_DATA fileinfo;
  HANDLE hFindFile = FindFirstFile(wildcard.c_str(), &fileinfo);
  if (hFindFile != INVALID_HANDLE_VALUE) {
    do {
      string subdir = directory;
      subdir += fileinfo.cFileName;
      subdir = ToLower(subdir);
      list<string>::const_iterator r = find(excludeDirList.begin(), excludeDirList.end(), subdir);
      if (r == excludeDirList.end()) {
        if ((fileinfo.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) != 0 &&
                                         fileinfo.cFileName[0] != '.' &&
                                         stricmp(fileinfo.cFileName, "RECYCLER") != 0) {
          subdir += '\\';

          foundAll = true;
          vector<Feature>::iterator feature;
          for (feature = features.begin(); feature != features.end(); feature++) {
            if (!feature->Locate(subdir.c_str()))
              foundAll = false;
          }

          if (foundAll)
            break;
          TreeWalk(subdir);
        }
      }
    } while (FindNextFile(hFindFile, &fileinfo));

    FindClose(hFindFile);
  }

  return foundAll;
}


bool ProcessHeader(const string & headerFilename)
{
  string inFilename = headerFilename;
  inFilename += ".in";

  ifstream in(inFilename.c_str(), ios::in);
  if (!in.is_open()) {
    cerr << "Could not open " << inFilename << endl;
    return false;
  }

  ofstream out(headerFilename.c_str(), ios::out);
  if (!out.is_open()) {
    cerr << "Could not open " << headerFilename << endl;
    return false;
  }

  while (in.good()) {
    string line;
    getline(in, line);

    vector<Feature>::iterator feature;
    for (feature = features.begin(); feature != features.end(); feature++)
      feature->Adjust(line);

    out << line << '\n';
  }

  return !in.bad() && !out.bad();
}

BOOL FeatureEnabled(const string & name)
{
  vector<Feature>::iterator feature;
  for (feature = features.begin(); feature != features.end(); feature++) {
    Feature & f = *feature;
    if (f.featureName == name && f.enabled)
      return TRUE;
  }
  return FALSE;
}

int main(int argc, char* argv[])
{
	// open and scan configure.ac
	cout << "PWLib Configure " VERSION " - ";
	ifstream conf("configure.ac", ios::in);
	if (conf.is_open()) {
		cout << "opened configure.ac" << endl;
	} else {
		conf.clear();
		conf.open("configure.in", ios::in);
		if (conf.is_open()) {
			cout << "opened " << "configure.in" << endl;
		} else {
			cerr << "could not open configure.ac/configure.in" << endl;
			return 1;
		}
	}

	// scan for features
  list<string> headers;
  vector<Feature>::iterator feature;

  while (conf.good()) {
    string line;
    getline(conf, line);
    int pos;
    if ((pos = line.find("AC_CONFIG_HEADERS")) != string::npos) {
      if ((pos = line.find('(', pos)) != string::npos) {
        int end = line.find(')', pos);
        if (end != string::npos)
          headers.push_back(line.substr(pos+1, end-pos-1));
      }
    }
    else if ((pos = line.find("dnl MSWIN_")) != string::npos) {
      int space = line.find(' ', pos+10);
      if (space != string::npos) {
        string optionName(line, pos+10, space-pos-10);
        while (line[space] == ' ')
          space++;
        int comma = line.find(',', space);
        if (comma != string::npos) {
          string optionValue(line, comma+1, INT_MAX);
          if (!optionValue.empty()) {
            string featureName(line, space, comma-space);
            bool found = false;
            for (feature = features.begin(); feature != features.end(); feature++) {
              if (feature->featureName == featureName) {
                found = true;
                break;
              }
            }
            if (found)
              feature->Parse(optionName, optionValue);
            else
              features.push_back(Feature(featureName, optionName, optionValue));
          }
        }
      }
    }
  }

	// scan version.h if there is a feature called version and if version.h exists
	for (feature = features.begin(); feature != features.end(); ++feature)
		if (feature->featureName == "version")
			break;
	if (feature != features.end()) {
		ifstream version("version.h", ios::in);
		if (version.is_open()) {
			while (version.good()) {
				string line;
				getline(version, line);
				int pos;
				int i;
				for (i = 0; i < (sizeof(VersionTags)/sizeof(VersionTags[0])); ++i) {
					int tagLen = strlen(VersionTags[i]);
					if ((pos = line.find(VersionTags[i])) != string::npos) {
						int space = line.find(' ', pos+tagLen);
						if (space != string::npos) {
							while (line[space] == ' ')
								space++;
							string version = line.substr(space);
							while (::iswspace(version[0]))
								version.erase(0);
							while (version.length() > 0 && ::iswspace(version[version.length()-1]))
								version.erase(version.length()-1);
							feature->defineValues.insert(pair<string,string>(VersionTags[i], version));
						}
					}
				}
			}
			string version("\"");
			map<string,string>::iterator ver;
			if ((ver = feature->defineValues.find(VersionTags[0])) != feature->defineValues.end())
				version += ver->second;
			else
				version += "0";
			version += ".";
			if ((ver = feature->defineValues.find(VersionTags[1])) != feature->defineValues.end())
				version += ver->second;
			else
				version += "0";
			version += ".";
			if ((ver = feature->defineValues.find(VersionTags[2])) != feature->defineValues.end())
				version += ver->second;
			else
				version += "0";
			version += "\"";
			feature->defineValues.insert(pair<string,string>("VERSION", version));
		}
	}

  const char EXTERN_DIR[]  = "--extern-dir=";
  const char EXCLUDE_DIR[] = "--exclude-dir=";
  const char EXCLUDE_ENV[] = "--exclude-env=";

  bool searchDisk = true;
  char *externDir = NULL;
  char *externEnv = NULL;
	int i;
  for (i = 1; i < argc; i++) {
    if (stricmp(argv[i], "--no-search") == 0 || stricmp(argv[i], "--disable-search") == 0)
      searchDisk = false;
    else if (strnicmp(argv[i], EXCLUDE_ENV, sizeof(EXCLUDE_ENV) - 1) == 0){
        externEnv = argv[i] + sizeof(EXCLUDE_ENV) - 1;
    }
    else if (strnicmp(argv[i], EXTERN_DIR, sizeof(EXTERN_DIR) - 1) == 0){
        externDir = argv[i] + sizeof(EXTERN_DIR) - 1;
    }
    else if (strnicmp(argv[i], EXCLUDE_DIR, sizeof(EXCLUDE_DIR) - 1) == 0) {
      string dir(argv[i] + sizeof(EXCLUDE_DIR) - 1);
      excludeDirList.push_back(dir);
      cout << "Excluding " << dir << " from feature search" << endl;
    }
    else if (stricmp(argv[i], "-v") == 0 || stricmp(argv[i], "--version") == 0) {
      cout << "configure version " VERSION "\n";
      return 1;
    }
    else if (stricmp(argv[i], "-h") == 0 || stricmp(argv[i], "--help") == 0) {
      cout << "configure version " VERSION "\n"
              "usage: configure args\n"
              "  --no-search           Do not search disk for libraries.\n"
              "  --extern-dir=dir      Specify where to search disk for libraries.\n"
              "  --exclude-dir=dir     Exclude dir from search path.\n";
              "  --exclude-env=var     Exclude dirs decribed by specified env var from search path.\n";
      for (feature = features.begin(); feature != features.end(); feature++) {
        if (feature->featureName[0] != '\0') {
            cout << "  --disable-" << feature->featureName
                 << setw(20-feature->featureName.length()) << "Disable " << feature->displayName << '\n';
          if (!feature->checkFiles.empty())
            cout << "  --" << feature->featureName << "-dir=dir"
                 << setw(30-feature->featureName.length()) << "Set directory for " << feature->displayName << '\n';
        }
      }
      return 1;
    }
    else {
			// parse environment variable if it exists
			std::vector<std::string> envConfigureList;
			char * envStr = getenv("PWLIB_CONFIGURE_FEATURES");
			if (envStr != NULL) {
				string str(envStr);
				string::size_type offs = 0;
				while (offs < str.length()) {
					string::size_type n = str.find(',', offs);
					string xable;
					if (n != string::npos) {
						xable = str.substr(offs, n-offs);
						offs = n+1;
					} else {
						xable = str.substr(offs);
						offs += str.length();
					}
					envConfigureList.push_back(ToLower(xable));
				}
			}

			// go through features and disable ones that need disabling either from command line or environment
      for (feature = features.begin(); feature != features.end(); feature++) {
        if (stricmp(argv[i], ("--no-"     + feature->featureName).c_str()) == 0 ||
            stricmp(argv[i], ("--disable-"+ feature->featureName).c_str()) == 0) {
          feature->enabled = false;
          break;
        }
        else if (stricmp(argv[i], ("--enable-"+ feature->featureName).c_str()) == 0) {
          feature->enabled = true;
          break;
        }
				else if (find(envConfigureList.begin(), envConfigureList.end(), "enable-"+ feature->featureName) != envConfigureList.end()) {
          feature->enabled = true;
          break;
				}
				else if (find(envConfigureList.begin(), envConfigureList.end(), "disable-"+ feature->featureName) != envConfigureList.end() ||
								 find(envConfigureList.begin(), envConfigureList.end(), "no-"+ feature->featureName) != envConfigureList.end()) {
          feature->enabled = false;
          break;
				}
        else if (strstr(argv[i], ("--" + feature->featureName + "-dir=").c_str()) == argv[i] &&
               !feature->Locate(argv[i] + strlen(("--" + feature->featureName + "-dir=").c_str())))
          cerr << feature->displayName << " not found in "
               << argv[i] + strlen(("--" + feature->featureName+"-dir=").c_str()) << endl;

      }
    }
  }

  for (i = 0; i < 2; i++) {
    char * env = NULL;
    switch (i) {
      case 0: 
        env = getenv("PWLIB_CONFIGURE_EXCLUDE_DIRS");
        break;
      case 1: 
        if (externEnv != NULL)
          env = getenv(externEnv);
        break;
    }
    if (env != NULL) {
      string str(env);
      string::size_type offs = 0;
      while (offs < str.length()) {
        string::size_type n = str.find(';', offs);
        string dir;
        if (n != string::npos) {
          dir = str.substr(offs, n-offs);
          offs = n+1;
        } else {
          dir = str.substr(offs);
          offs += str.length();
        }
        excludeDirList.push_back(ToLower(dir));
        cout << "Excluding " << dir << " from feature search" << endl;
      }
    }
  }

  bool foundAll = true;
  for (feature = features.begin(); feature != features.end(); feature++) {
    if (feature->enabled && !feature->checkFiles.empty()) {
      bool foundOne = false;
      list<string>::iterator dir;
      for (dir = feature->checkDirectories.begin(); dir != feature->checkDirectories.end(); dir++) {
        if (feature->Locate(dir->c_str())) {
          foundOne = true;
          break;
        }
      }
      if (!foundOne)
        foundAll = false;
    }
  }

  if (searchDisk && !foundAll) {
    // Do search of entire system
    char drives[100];
    if (!externDir){
      if (!GetLogicalDriveStrings(sizeof(drives), drives))
        strcpy(drives, "C:\\");
    }
    else {
      strcpy(drives, externDir);
    }

    const char * drive = drives;
    while (*drive != '\0') {
      if (GetDriveType(drive) == DRIVE_FIXED) {
        cout << "Searching " << drive << endl;
        if (TreeWalk(drive))
          break;
      }
      drive += strlen(drive)+1;
    }
  }

  for (feature = features.begin(); feature != features.end(); feature++) {
    cout << "  " << feature->displayName << ' ';
    BOOL output = FALSE;
    list<string>::const_iterator r;
    if (feature->enabled) {
      for (r = feature->ifFeature.begin(); r != feature->ifFeature.end(); ++r) {
				string str = *r;
				size_t pos = str.find('|');
				if (pos == string::npos) {
					if (!FeatureEnabled(str)) {
						feature->enabled = FALSE;
						cout << " DISABLED due to absence of feature " << str;
						output = TRUE;
						break;
					}
				}
				else
				{
					bool enabled = FALSE;
					while (str.length() != 0) {
						string key = str.substr(0, pos);
						str = str.substr(pos+1);
						if (FeatureEnabled(key)) {
							enabled = true;
							break;
						}
						pos = str.find('|');
					}
					if (!enabled) {
						feature->enabled = FALSE;
						cout << " DISABLED due to absence of any features in list " << *r;
						output = TRUE;
					}
				}
      }
    }
    if (feature->enabled) {
      for (r = feature->ifNotFeature.begin(); r != feature->ifNotFeature.end(); ++r) {
        if (FeatureEnabled(*r)) {
          feature->enabled = FALSE;
          cout << " DISABLED due to presence of feature " << *r;
          output = TRUE;
          break;
        }
      }
     if (!feature->checkFiles.empty() && !feature->checkFiles.begin()->found)
       feature->enabled = FALSE;
    }
    if (output)
      ;
    else if (feature->checkFiles.empty() && !feature->simpleDefineValue.empty())
      cout << "set to " << feature->simpleDefineValue;
    else if (feature->enabled) 
      cout << "enabled";
    else 
      cout << "DISABLED";
    cout << '\n';
  }
  cout << endl;

  for (list<string>::iterator hdr = headers.begin(); hdr != headers.end(); hdr++)
    ProcessHeader(*hdr);

  cout << "Configuration completed.\n";

  return 0;
}
