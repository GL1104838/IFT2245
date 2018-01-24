/* ch.c.
auteur: Gabriel Lemyre/Kevin Belisle
date: 24-01-2018
problèmes connus:

*/

#include  <sys/types.h>
#include  <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <vector>
#include <regex>
#include <fstream>

using namespace std;

void requestInput();
void readInput(string);
void readKeywords();
void readEcho(string);
void forLoop(string, vector<string>);
bool execCommand(vector<string>);

//Contains parsed words
vector<string> stringQueue;

//Contains the instructions after the "done" input in a for loop
vector<string> instructionsAfterLoop;

//Contains variables
vector<vector<string>> variables;

//Will be shown to the user
string outputString;

bool invalidInput = false;

int main()
{
	cout << "%% ";

	/* ¡REMPLIR-ICI! : Lire les commandes de l'utilisateur et les exécuter. */
	requestInput();

	cout << "Bye!\n";
	return 0;
}

bool execCommand(vector<string> args)
{
    //Transform vector<string> into const char** and add NULL at the end
    vector<char*> argc;
    argc.reserve(args.size()+1);
    for (int i = 0; i < args.size(); i++)
    {
        argc.push_back(const_cast<char*>(args[i].c_str()));
    }
    //Args need to end with NULL terminator
    argc[args.size()] = NULL;

    //str.c_str();
    id_t pid;
    siginfo_t state;

    pid = fork();
    if (pid == 0)
    {
        /* Child Process */
        execvp(argc[0], &argc[0]);
        exit(1);//if we reach this, an error has occured.
    }
    else if (pid > 0)
    { 
        /* Parent Process */
        int result = waitid(P_PID, pid, &state, WEXITED);
        if (result == 0)
        {
            /*Process ended correctly*/
            return true;
        }
        else
        {
            /*Process ended incorrectly*/
            outputString = "Thread hasn't ended properly!";
            return false;
        }
    }
    else
    {
        /* Error while fork()ing */
        outputString = "Error while forking!\n";
        return false;
    }
}

void requestInput() {
	//Called after every instruction
	
	string inputString;
	cout << "Shell> ";
	
	getline(cin, inputString);
	readInput(inputString);
	
	if (invalidInput) {
		outputString = "Invalid input error.\n";
	}
	cout << outputString;
	outputString = "";
	stringQueue.clear();
	invalidInput = false;
	requestInput();
}

void readInput(string inputString) {
	//Reads the input

	//Read the words and put them in the stringQueue
	string temp;
	for (int i = 0; i < inputString.length(); i++) {
		if (!(inputString.at(i) == ' ')) {
			//if the char is not an empty space or a ';'
			temp.append(string(1, inputString.at(i)));
		}
		else {
			//if the char is an empty space or a ';'
			if (temp != "") {
				//if temp is not empty
				stringQueue.push_back(temp);
			}
			temp = "";
		}
	}
	if (temp != "") {
		//pushes the last word
		stringQueue.push_back(temp);
	}
	readKeywords();
}

void forLoop(string iterator, vector<string> loopSet) {
	if (loopSet.size() > 0) {

		bool variableIsDefined = false;
		for (int i = 0; i < variables.size(); i++) {
			if (iterator == variables.at(i).at(0)) {
				variableIsDefined = true;
				variables.at(i).at(1) = loopSet.at(0);
			}
		}
		if (!variableIsDefined) {
			vector<string> loopVar;
			loopVar.push_back(iterator);
			loopVar.push_back(loopSet.at(0));
			variables.push_back(loopVar);
		}
		readKeywords();
		loopSet.erase(loopSet.begin());
		forLoop(iterator, loopSet);
	}
}

void readKeywords() {
	bool commandFailed = false;
	//Reads the keywords from the stringQueue
    for (int i = 0; i < stringQueue.size(); i++) {

        string currentKeyword = stringQueue.at(i);

        if (currentKeyword == "echo" ||
            currentKeyword == "cat" || 
            currentKeyword == "ls" ||
            currentKeyword == "man" ||
            currentKeyword == "tail") {
            vector<string> args;  
            args.push_back(stringQueue.at(i));
			//if the current keyword is a command
			for (int j = i + 1; j < stringQueue.size(); j++) {
				if (stringQueue.at(j) == ";" || stringQueue.at(j) == "||" 
					|| stringQueue.at(j) == "&&") {
					//stop operation
					j = stringQueue.size();
				}
				else {
					//Variables check
					currentKeyword = stringQueue.at(j);
					if (currentKeyword.at(0) == '$') {
						string stringVariable = 
						   currentKeyword.substr(1, currentKeyword.size() - 1);
						for (int k = 0; k < variables.size(); k++) {
							if (stringVariable == variables.at(k).at(0)) {
                                args.push_back(variables.at(k).at(1));
							}
						}
					}
					else {
                        args.push_back(stringQueue.at(j));
					}
					i++;
				}
			}
            commandFailed = execCommand(args);
            outputString = "\n";
		}
		else if (currentKeyword.at(0) == '$') {
			//if the current keyword is variable command
			string stringVariable = 
				currentKeyword.substr(1, currentKeyword.size() - 1);
			for (int j = 0; j < variables.size(); j++) {
				if (stringVariable == variables.at(j).at(0)) {
					stringQueue.at(i) = variables.at(j).at(1);
					i--;
				}
			}
		}
		else if (currentKeyword == "for") {
			//if the current keyword is for
			vector<string> loopSet;
			string iterator;

			if (i + 1 < stringQueue.size()) {
				//Initialize the iterator
				iterator = stringQueue.at(i + 1);

				if (i + 2 < stringQueue.size()) {
					if (stringQueue.at(i + 2) == "in") {
						for (int j = i + 3; j < stringQueue.size(); j++) {
							if (stringQueue.at(j) == ";") {
								//end of the loopset
								stringQueue.erase(stringQueue.begin(), 
									stringQueue.begin() + j+1);
								j = stringQueue.size();
							}
							else {
								loopSet.push_back(stringQueue.at(j));
							}
						}
					}
					else {
						outputString = "Missing 'in' statement\n";
					}
				}
			}
			if (stringQueue.size() > 0) {
				//if there is a do statement
				if (stringQueue.at(0) == "do") {
					stringQueue.erase(stringQueue.begin());

					bool addInstructions = false;
					for (int j = 0; j < stringQueue.size(); j++) {
						if (addInstructions) {
							instructionsAfterLoop.push_back(stringQueue.at(j));
							stringQueue.erase(stringQueue.begin() + j);
							j--;
						}
						else if (stringQueue.at(j) == "done") {
							stringQueue.erase(stringQueue.begin() + j);
							j--;
							addInstructions = true;
						}
					}
					if (addInstructions) {
						forLoop(iterator, loopSet);
						//loop is completed
						stringQueue = instructionsAfterLoop;
						instructionsAfterLoop.clear();
						i = -1;

						//delete iterator from variables
						for (int j = 0; j < variables.size(); j++) {
							if (variables.at(j).at(0) == iterator) {
								variables.erase(variables.begin() + j);
								j = variables.size();
							}
						}
					}
					else {
						outputString = "Missing 'done' statement\n";
					}
				}
				else {
					outputString = "Missing 'do' statement\n";
				}
			}
		}
		else if (currentKeyword == "||") {
			//Defines if the following text is to be executed
			if (!commandFailed) {
				bool interruptedLoop = false;
				for (int j = i+1; j < stringQueue.size(); j++) {
					if (stringQueue.at(j) == ";" || stringQueue.at(j) == "||" 
						|| stringQueue.at(j) == "&&") {
						i = j-1;
						j = stringQueue.size();
						interruptedLoop = true;
					}
				}
				if (!interruptedLoop) {
					i = stringQueue.size();
				}
			}
			commandFailed = false;
		}
		else if (currentKeyword == "&&") {
			if (commandFailed) {
				bool interruptedLoop = false;
				for (int j = i + 1; j < stringQueue.size(); j++) {
					if (stringQueue.at(j) == ";" || stringQueue.at(j) == "||" 
						|| stringQueue.at(j) == "&&") {
						i = j-1;
						j = stringQueue.size();
						interruptedLoop = true;
					}
				}
				if (!interruptedLoop) {
					i = stringQueue.size();
				}
			}
		}
		else if (currentKeyword != ";"){
			//Verify if it is a variable assignment
			bool isVariableAssigment = false;
			int equalSignPos;
			for (int j = 0; j < currentKeyword.size(); j++) {
				if (currentKeyword.at(j) == '=') {
					isVariableAssigment = true;
					equalSignPos = j;
					j = currentKeyword.size();
				}
			}
			if (isVariableAssigment) {
				//Assign the variable or modify it
				bool modifiedVariable = false;
				vector<string> currentVar;
				string currentStr = currentKeyword.substr(0, equalSignPos);

				for (int j = 0; j < variables.size(); j++) {
					if (variables.at(j).at(0) == currentStr) {
						//Modify the variable
						modifiedVariable = true;
						variables.at(j).at(1) =
							(currentKeyword.substr(
								equalSignPos + 1, currentKeyword.size() - 1));
					}
				}
				if (!modifiedVariable) {
					//assign the variable
					currentVar.push_back(
						currentKeyword.substr(0, equalSignPos));
					currentVar.push_back(
						currentKeyword.substr(
							equalSignPos + 1, currentKeyword.size() - 1));
					variables.push_back(currentVar);
				}
			}
			else {
				//The input is invalid
				invalidInput = true;
				i = stringQueue.size();
			}
		}
		else if (currentKeyword == ";") {
			commandFailed = false;
		}
	}
}