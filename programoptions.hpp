#include <iostream>
#include <fstream>
#include <memory>
#include <limits>
#include <algorithm>
#include <string>
#include <sstream>
#include <stdexcept>
#include <map>
#include <vector>
using namespace std;
// class ProgramOptions 
// {
//     class Cmdoption {
//         public:
//             string help;
//             vector <string> parser;
//             string parser_raw;
//             void showHelp();
//             template <class T> 
//             Cmdoption(string parser, T default_value, string help);
//             template <class T> 
//             T getVal(){

//             }


//     };
//     template <class T>
//     class CmdoptionImpl {
//         public:
//             T val;
//             T default_value;

//             Cmdoption(string parser, T default_value, string help){
//                 this->val = default_value;
//                 this->default_value = default_value;
//                 this->help = help;
//                 this->parser = obtainParse(parser);
//                 this->parser_raw = parser;
//             }
//             void showHelp(){
//                 cout << parser_raw << " " << help << " default: "<<default_value<< endl;
//             }
//             T getVal(){
//                 return this->val;
//             }
//         private:
//             vector <string> obtainParse(string parse){
//                 stringstream ss(parse);
//                 string token;
//                 vector <string> result = vector <string> ();
//                 while(getline(ss,token, ',')) result.push_back(token);
//                 return result;

//             }
//     } ; 
//     public:
//         string title;
//         vector <string> arguments;
//         ProgramOptions(string title){
//             this->title = title;
//             commands = vector <variant <int, string, float> defaultval > ();
//             cmd_map = map <string, variant <int, string, float> defaultval > ();
//             addOption("mode,m", "dfd", "Operation mode (dfd, merge)");
//             addOption("input,i", "", "input filename");
//             addOption("output,o", "shots.txt", "output filename");
//             addOption("start,s", 0, "start frame (default: 0)");
//             addOption("end,e", -1,  "end frame (default: -1 for full length of video)");
//             addOption("threshold,t", 2e8,  "threshold");
//         }

//         template <class cmdType>
//         void addOption(string parse, variant <int, string, float> defaultval, string instructions){
//             Cmdoption <cmdType> *command = new Cmdoption <cmdType> (parse, defaultval, instructions);
//             std::unique_ptr<CmdoptionParent> command_ptr = std::unique_ptr<CmdoptionParent>(command);
//             commands.push_back(command_ptr);
//             for (int i = 0; i < command->parser.size(); i++){
//                 string key_val = "-"+(command->parser[i]);
//                 cmd_map[key_val] = command_ptr;
//             }
//         }
//         void showHelp(){
//             cout << this->title << endl;
//             for (int c_id = 0; c_id < commands.size(); c_id++) commands[c_id]->showHelp();
//         }
//         template <class cmdType>
//         cmdType getOption(string command_name){
//             return Cmdoption <cmdType> (cmd_map[command_name].get()).getVal();
//         }
//         void parseCommands(int argc, char *argv[]){
//             if (argc >= 2){
//                 for (int i = 2; i < argc; i+=2){
//                     string curr_arg = argv[i];

//                     arguments.push_back(curr_arg);
//                     if (curr_arg == "-h"){
//                         return;
//                     }
//                     else {
//                         if (cmd_map.count(curr_arg) > 0){
//                             // TODO: parse here
//                         }
//                     }
//                 }
//             }

//         }
//     private:
//         vector <std::unique_ptr<CmdoptionParent> > commands;
//         map <string, std::unique_ptr<CmdoptionParent> > cmd_map;

// };


class ProgramOptions 
{
    public:
        string mode;
        string input;
        string output;
        int start;
        int end;
        int threshold;
        string title;
        ProgramOptions(string title){
            this->title = title;
            this->mode = "dfd";
            this->output = "shots.txt";
            this->start = 0;
            this->end = -1;
            this->threshold = 2e8;

        }

};
        