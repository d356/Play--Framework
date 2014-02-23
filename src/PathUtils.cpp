#include "PathUtils.h"

using namespace Framework;

#ifdef _WIN32

#if WINAPI_FAMILY_PARTITION(WINAPI_FAMILY_APP)

boost::filesystem::path PathUtils::GetPersonalDataPath()
{
	auto localFolder = Windows::Storage::ApplicationData::Current->LocalFolder;
	return boost::filesystem::path(localFolder->Path->Data());
}

#else	// !WINAPI_PARTITION_APP

#include <shlobj.h>

boost::filesystem::path PathUtils::GetPathFromCsidl(int csidl)
{
	wchar_t userPathString[MAX_PATH];
	if(FAILED(SHGetFolderPathW(NULL, csidl, NULL, 0, userPathString)))
	{
		throw std::runtime_error("Couldn't get path from csidl.");
	}
	return boost::filesystem::wpath(userPathString, boost::filesystem::native);
}

boost::filesystem::path PathUtils::GetRoamingDataPath()
{
	return GetPathFromCsidl(CSIDL_APPDATA);
}

boost::filesystem::path PathUtils::GetPersonalDataPath()
{
	return GetPathFromCsidl(CSIDL_PERSONAL);
}

boost::filesystem::path PathUtils::GetAppResourcesPath()
{
	return boost::filesystem::path(".");
}

#endif	// !WINAPI_PARTITION_APP

#endif	// _WIN32

#if defined(_POSIX_VERSION)

#if defined(__APPLE__)

#include <CoreFoundation/CoreFoundation.h>
#include <Foundation/Foundation.h>

boost::filesystem::path PathUtils::GetSettingsPath()
{
	NSArray* paths = NSSearchPathForDirectoriesInDomains(NSLibraryDirectory, NSUserDomainMask, YES);
	std::string directory = [[paths objectAtIndex: 0] UTF8String];
	return boost::filesystem::path(directory);
}

boost::filesystem::path PathUtils::GetRoamingDataPath()
{
	NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
	std::string directory = [[paths objectAtIndex: 0] UTF8String];
	return boost::filesystem::path(directory);
}

boost::filesystem::path PathUtils::GetAppResourcesPath()
{
	NSBundle* bundle = [NSBundle mainBundle];
	NSString* bundlePath = [bundle resourcePath];
	return boost::filesystem::path([bundlePath UTF8String]);
}

boost::filesystem::path PathUtils::GetPersonalDataPath()
{
	return GetRoamingDataPath();
}

#else	// DEFINED(__APPLE__)

#include <pwd.h>

boost::filesystem::path PathUtils::GetPersonalDataPath()
{
	passwd* userInfo = getpwuid(getuid());
	return boost::filesystem::path(userInfo->pw_dir);
}

#endif	// !DEFINED(__APPLE__)

#endif	// !DEFINED(_POSIX_VERSION)

void PathUtils::EnsurePathExists(const boost::filesystem::path& path)
{
	typedef boost::filesystem::path PathType;
	PathType buildPath;
	for(PathType::iterator pathIterator(path.begin());
		pathIterator != path.end(); pathIterator++)
	{
		buildPath /= (*pathIterator);
		boost::system::error_code existsErrorCode;
		bool exists = boost::filesystem::exists(buildPath, existsErrorCode);
		if(existsErrorCode)
		{
#ifdef WIN32
			if(existsErrorCode.value() == ERROR_ACCESS_DENIED)
			{
				//No problem, it exists, but we just don't have access
				continue;
			}
			else if(existsErrorCode.value() == ERROR_FILE_NOT_FOUND)
			{
				exists = false;
			}
#else
			if(existsErrorCode.value() == ENOENT)
			{
				exists = false;
			}
#endif
			else
			{
				throw std::runtime_error("Couldn't ensure that path exists.");
			}
		}
		if(!exists)
		{
			boost::filesystem::create_directory(buildPath);
		}
	}
}
