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

int main() {
    vector<ComponentInfo> components;

    ifstream myfile;
    string line;
    myfile.open("components-list.txt");
    unsigned int counter = 0;
    while (getline(myfile,line)) {
        ComponentInfo component;
        component.id = counter++;
        component.name = line;
        components.push_back(component);
        cout << line << ", " << component.id << '\n';
    }
    myfile.close();

    
    ostringstream stream;

    for (auto component : components) {
        stream << "template<> constexpr unsigned int getRawID<const " << component.name << ">() { return " << component.id << "; }\n";   
    }

    cout << stream.str();

    ofstream output("_componentIDs.hpp");

    output << stream.str();
    output.close();

    ofstream macroOut("macro.hpp");

    ostringstream macro;
    macro << "#define COMPONENTS ";
    for (size_t i = 0; i < components.size(); i++) {
        ComponentInfo component = components[i];
        macro << component.name;
        if (i != components.size() - 1)
            macro << ", ";
    }

    macroOut << macro.str();
    macroOut << "\n#define NUM_COMPONENTS " << components.size() << "\n";
    macroOut.close();

    ofstream componentDecl("componentDecl.hpp");
    for (size_t i = 0; i < components.size(); i++) {
        auto component = components[i];
        componentDecl << "struct " << component.name << ";\n";
    }

    return 0;
}