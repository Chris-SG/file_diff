#include "stdafx.h"

#include <string>
#include <filesystem>
#include <vector>
#include <iostream>

#include <openssl/md5.h>
#include <chrono>

namespace fs = std::experimental::filesystem;

// status of each file
enum s_filestatus
{
	unchanged,
	modified,
	deleted,
	added
};

// file differences
struct s_filediff
{
	fs::path name;
	s_filestatus status;
};

std::vector<s_filediff> get_diff(const fs::path origpath, const fs::path newpath);
std::string get_md5(fs::path a_filepath);

// holds the program running path
fs::path g_initial_path;


int main(const int argc, const char* argv[])
{
	auto start = std::chrono::system_clock::now();
	// get program running path
	g_initial_path = fs::current_path();
	// requires at least an old path and a new path
	if(argc < 3)
	{
		return 1;
	}
	// ensure directories exist
	if(fs::exists(argv[1]) && fs::exists(argv[2]))
	{
		// store args
		fs::path l_oldpath = argv[1];
		fs::path l_newpath = argv[2];
		// set paths to absolute
		if (l_oldpath.is_relative())
			l_oldpath = fs::canonical(l_oldpath);
		if (l_newpath.is_relative())
			l_newpath = fs::canonical(l_newpath);
		// perform get_diff
		auto l_diff = get_diff(l_oldpath, l_newpath);

		// loop over all diff files
		/*for(auto & p : l_diff)
		{
			std::cout << g_initial_path <<p.name << "   " << p.status << std::endl;
		}*/
		int count0 =0, count1 =0, count2 =0, count3 = 0;
		for(auto & p : l_diff)
		{
			switch(p.status)
			{
			case 0:
			{
				++count0;
				break;
			}
			case 1:
				{
				++count1;
				break;
				}
			case 2:
				{
				++count2;
				break;
				}
			case 3:
				++count3;
				
			}
		}
		std::cout << "Unchanged: " << count0 << std::endl << "Modified: " << count1 << std::endl << "Deleted: " << count2 << std::endl << "Added: " << count3 << std::endl;
	}
	else
	{
		// directory paths are invalid
		std::cout << "One or more paths do not exist!" << std::endl;
		return 2;
	}
	std::cout << "Running time: " << (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start)).count() << "ms" << std::endl;
    return 0;
}

// compare two directories
std::vector<s_filediff> get_diff(const fs::path a_oldpath, const fs::path a_newpath)
{
	// return vector
	std::vector<s_filediff> l_ret;
	// directory entry vectors
	std::vector<fs::directory_entry> l_oldfiles, l_newfiles;
	// set path to oldpath
	fs::current_path(a_oldpath);
	// iterate all files in path
	for (auto & p : fs::recursive_directory_iterator("."))
	{
		// only add file if not a directory
		if (fs::is_regular_file(p))
			l_oldfiles.push_back(p);
	}
	// reset path to initial path
	fs::current_path(g_initial_path);
	// set path to newpath
	fs::current_path(a_newpath);
	// iterate all files in path
	for (auto & p : fs::recursive_directory_iterator("."))
	{
		// only add file if not a directory
		if(fs::is_regular_file(p))
			l_newfiles.push_back(p);
	}
	// reset path to initial path
	fs::current_path(g_initial_path);
	// iterate over all oldfiles
	for (auto & op : l_oldfiles)
	{
		// search for oldfile name in newfile vector
		const auto np = std::find(l_newfiles.begin(), l_newfiles.end(), op);
		// if we have a filename match
		if (np != l_newfiles.end())
		{
			// compare file sizes
			if(fs::file_size(fs::path(a_oldpath) / op) != fs::file_size(fs::path(a_newpath) / *np))
			{
				// if file size is different, we can be sure the file has been modified
				const s_filediff diff = { op, modified };
				l_ret.push_back(diff);
				// remove filename from new vector
				l_newfiles.erase(std::find(l_newfiles.begin(), l_newfiles.end(), *np));
			}
			else
			{
				if(get_md5(fs::path(a_oldpath) / op) != get_md5(fs::path(a_oldpath) / *np))
				{
					// if file md5 is different, we can be sure the file has been modified
					const s_filediff diff = { op, modified };
					l_ret.push_back(diff);
					// remove filename from new vector
					l_newfiles.erase(std::find(l_newfiles.begin(), l_newfiles.end(), *np));
				}
				else
				{
					// if file md5 is the same, we can be sure the file is not modified
					const s_filediff diff = { op, unchanged };
					l_ret.push_back(diff);
					// remove filename from new vector
					l_newfiles.erase(std::find(l_newfiles.begin(), l_newfiles.end(), *np));
				}
			}
		}
		else
		{
			// the file has been deleted
			const s_filediff diff = { op, deleted };
			l_ret.push_back(diff);
		}
	}
	// the rest of the files remaining in l_newfiles are new
	for(const auto a : l_newfiles)
	{
		const s_filediff diff = { a, added };
		l_ret.push_back(diff);
	}
	// return our vector
	return l_ret;
}

// pretty much taken from https://stackoverflow.com/questions/10324611/how-to-calculate-the-md5-hash-of-a-large-file-in-c
std::string get_md5(fs::path a_filepath)
{
	std::string remove("\\.");
	auto l_filepath = a_filepath.string();
	const auto i = l_filepath.find(remove);
	if (i != std::string::npos)
		l_filepath.erase(i, remove.length());
	unsigned char c[MD5_DIGEST_LENGTH];
	FILE *l_infile = fopen(reinterpret_cast<char const*>(l_filepath.c_str()), "rb");
	MD5_CTX l_mdcontext;
	int bytes;
	unsigned char data[1024];

	if(l_infile == nullptr)
	{
		std::cout << "File cannot be opened: " << l_filepath << std::endl;
		return std::string();
	}

	MD5_Init(&l_mdcontext);
	while ((bytes = fread(data, 1, 1024, l_infile)) != 0)
		MD5_Update(&l_mdcontext, data, bytes);
	MD5_Final(c, &l_mdcontext);
	std::string md5_ret;
	for(std::size_t i = 0; i != 16; i++)
	{
		md5_ret += "0123456789ABCDEF"[c[i] / 16];
		md5_ret += "0123456789ABCDEF"[c[i] % 16];
	}
	fclose(l_infile);
	return md5_ret;
}