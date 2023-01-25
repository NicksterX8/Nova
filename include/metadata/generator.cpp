#include <stdio.h>
#include <string>
#include <vector>

#include <iostream>
#include <fstream>
#include <ostream>
#include <sstream>

using namespace std;

struct ComponentInfo {
    unsigned int id;
    string name;
};

bool startsWith(string str, const char* sub) {
    auto length = strlen(sub);
    auto substr = str.substr(0, length);
    return substr == sub;
}

string getDirectiveValue(string directive) {
    return directive.substr(directive.find_first_of('=') + 1);
}

int main(int argc, char** argv) {
    string root;
    string list;

    if (argc >= 3) {
        list = string(argv[1]); // input
        root = string(argv[2]); // output
    } else {
        printf("Not enough arguments!\n");
        exit(1);
    }

    vector<ComponentInfo> components;

    string functionName, macroPrefix, _namespace, _namespaceExtended;

    ifstream myfile(list);

    int counter = 0;
    string line;
    while (getline(myfile,line)) {
        if (line[0] == '#') { // directive
            string cmd = line.substr(1);
            string input = "ERROR";
            #define DIRECTIVE(name)input = getDirectiveValue(cmd); if (startsWith(cmd, name))
            DIRECTIVE("startCount") {
                counter = stoi(input);
            }
            DIRECTIVE("namespace") {
                _namespace = input;
                _namespaceExtended = _namespace + "::";
            }
            DIRECTIVE("function_name") {
                functionName = input;
            }
            DIRECTIVE("macro_prefix") {
                macroPrefix = input;   
            }
            if (input == "ERROR")
                printf("Unknown directive \"%s\"\n", cmd.c_str());
            continue;
        }

        ComponentInfo component;
        component.id = counter++;
        component.name = line;
        components.push_back(component);
        cout << line << ", " << component.id << '\n';
    }
    myfile.close();

    
    ostringstream stream;
    // idk whether to add constexpr here or not
    stream << "template<class C> constexpr ID_Type " << functionName << "() { return -1; }\n";
    for (auto component : components) {
        stream << "template<> constexpr ID_Type " << functionName << "<" << _namespaceExtended << component.name << ">() { return " << component.id << "; }\n";   
    }
    cout << stream.str();

    ofstream output(root + "ids.inc");

    output << stream.str();
    output.close();

    ofstream macroOut(root + "macro.hpp");

    ostringstream macro;
    ostringstream shortMacro;

    macro << "#define " << macroPrefix << "COMPONENTS ";
    for (size_t i = 0; i < components.size(); i++) {
        ComponentInfo component = components[i];
        macro << _namespaceExtended << component.name;
        if (i != components.size() - 1) {
            macro << ", ";
        }
    }

    shortMacro << "#define " << macroPrefix << "COMPONENT_NAMES ";
    for (size_t i = 0; i < components.size(); i++) {
        ComponentInfo component = components[i];
        shortMacro << component.name;
        if (i != components.size() - 1) {
            shortMacro << ", ";
        }
    }
    shortMacro << "\n";

    macroOut << macro.str();
    macroOut << "\n#define " << macroPrefix << "NUM_COMPONENTS " << components.size() << "\n";
    macroOut << shortMacro.str();
    macroOut.close();

    ofstream componentDecl(root + "declaration.hpp");
    componentDecl << "namespace " << _namespace << " {\n";
    for (size_t i = 0; i < components.size(); i++) {
        auto component = components[i];
        componentDecl << "struct " << component.name << ";\n";
    }
    componentDecl << "}";

    return 0;
}