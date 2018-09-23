

#include <iostream>
#include <fstream>
#include <string>
#include <set>
#include <map>
#include <experimental/filesystem>

using namespace std;
using namespace std::experimental::filesystem::v1;

using User=std::string;
using OsName=std::string;
using OSs=std::set<OsName>; 
using FingerprintDb=std::map<User, OSs>;

const string DEFAULT_DB_FILE_NAME("opensshenum.db");

void cmdErr(string cmdline){
    cerr << "Syntax:" << endl << cmdline << " [lst_files_dir_path]" << endl
         << "Where:" << endl 
         << "lst_files_dir_path     Full path of the lst files directory" << endl;
}

int main(int argc, char** argv){
    int  ret = 0;
    try{
        string path(argv[1]);
        FingerprintDb db;

        for(auto lstfilepath : directory_iterator(path)){
            cerr << "Reading: " << lstfilepath << endl;
            if(!is_regular_file(lstfilepath.path())){
                 cerr << "Not a regular file: skipping" << endl;
                 continue;
            }
            string lstfilepathstr = lstfilepath.path().string();
            string lstfilename    = lstfilepathstr.substr(lstfilepathstr.find_last_of("/") + 1);

            string ext(".lst");
            size_t checkExt =  lstfilename.rfind(ext);
            if( checkExt == string::npos || checkExt + ext.size() != lstfilename.size() ){
                 cerr << "Not .lst file: skipping" << endl;
                 continue;
            }

            string osname = lstfilename.substr(0,  lstfilename.size() - ext.size());

            cerr << "Reading: " << lstfilename << " Osname: " << osname << endl;
            fstream fs(lstfilepathstr, fstream::in); 
            string line;
            while(getline(fs, line, '\n')){
                cerr << "Reading element: " << line << " Osname: " << osname << endl;
                db.emplace(line, OSs());
                db[line].emplace(osname);
            }
            fs.close(); 
        }
        
        fstream fs(DEFAULT_DB_FILE_NAME, fstream::out); 
            for(auto line : db){
                fs << line.first << ";";
                for(auto elem : line.second)
                    fs << elem << ";";
                fs << endl;
            }
        fs.close(); 
    }catch(filesystem_error& err){
        cerr << err.what() << endl;
        cmdErr(argv[0]); 
        ret = 1;
    }

    return ret;
}
