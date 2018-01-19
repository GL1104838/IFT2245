/* ch.c.
auteur: Gabriel Lemyre
date: 18-01-2018
problèmes connus:

*/

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <vector>
#include <regex>
#include <fstream>
#include <dirent.h>

using namespace std;

void requestInput();
void readInput(string);
void readKeywords();
void readEcho(string);
void forLoop(string, vector<string>);

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

void requestInput(void) {
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

void listFiles() {
	//Lists all the files in the working directory
	struct dirent* dirent;
	DIR* currentDirectory = opendir(".");
	dirent = readdir(currentDirectory);

	while (dirent != NULL) {
		string dnameString = dirent->d_name;
		outputString.append(dnameString + "\n");
		dirent = readdir(currentDirectory);
	}
	closedir(currentDirectory);
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

		if (currentKeyword == "echo") {
			//if the current keyword is echo
			for (int j = i + 1; j < stringQueue.size(); j++) {
				if (stringQueue.at(j) == ";" || stringQueue.at(j) == "||" 
					|| stringQueue.at(j) == "&&") {
					//stop echo operation
					j = stringQueue.size();
				}
				else {
					//perform echo

					//Variables check
					currentKeyword = stringQueue.at(j);
					if (currentKeyword.at(0) == '$') {
						string stringVariable = 
						   currentKeyword.substr(1, currentKeyword.size() - 1);
						for (int k = 0; k < variables.size(); k++) {
							if (stringVariable == variables.at(k).at(0)) {
								outputString.append(variables.at(k).at(1));
								outputString.append(" ");
							}
						}
					}
					else {
						outputString.append(stringQueue.at(j) + " ");
					}
					i++;
				}
			}
			outputString.append("\n");
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
		else if (currentKeyword == "cat") {
			//if the current keyword is cat

			for (int j = i + 1; j < stringQueue.size(); j++) {
				if (stringQueue.at(j) == ";" || stringQueue.at(j) == "||" 
					|| stringQueue.at(j) == "&&") {
					//stop cat operation
					j = stringQueue.size();
				}
				else {
					//perform cat operation
					ifstream fileReader(stringQueue.at(j));
					string fileText;

					if (fileReader.is_open()) {
						while (getline(fileReader, fileText)) {
							outputString.append(fileText);
						}
						fileReader.close();
					}
					else {
						commandFailed = true;
						outputString = "Error, file not found.";
						//j = stringQueue.size();
					}
					i++;
				}
			}
			outputString.append("\n");
		}
		else if (currentKeyword == "ls") {
			//if the current keyword is ls
			if (i + 1 < stringQueue.size()) {
				if (stringQueue.at(i + 1) == ";") {
					listFiles();
					i++;
				}
			}
			else {
				listFiles();
			}
		}
		else if (currentKeyword == "tail") {
			//if the current keyword is tail

			vector<string> tailVector;
			int fileCount = 0;

			//Gets the amount of file to read
			for (int j = i + 1; j < stringQueue.size(); j++) {
				if (stringQueue.at(j) == ";") {
					j = stringQueue.size();
				}
				else {
					fileCount++;
				}
			}

			for (int j = i + 1; j < stringQueue.size(); j++) {
				if (stringQueue.at(j) == ";") {
					//stop tail operation
					i++;
					j = stringQueue.size();
				}
				else {
					try {
						//perform tail operation
						ifstream fileReader(stringQueue.at(j));
						string fileText;
						tailVector.clear();

						if (fileReader.is_open()) {
							while (getline(fileReader, fileText)) {
								tailVector.push_back(fileText);
							}
							fileReader.close();
						}
						else {
							outputString = 
								"Erreur, le fichier n'a pas pu être lu.";
							j = stringQueue.size();
						}

						//write to the outputString
						if (fileCount > 1) {
							outputString.append(
								"==> " + stringQueue.at(j) + " <==\n");
						}
						int loopValue = tailVector.size() - 10;
						if (loopValue < 0) {
							loopValue = 0;
						}
						for (int k = loopValue; k < tailVector.size(); k++) {
							//reads the 10 last lines of the file
							outputString.append(tailVector.at(k) + "\n");
						}

						i++;
					}
					catch (exception e) {
						outputString = 
							"Erreur, le fichier n'a pas pu être lu.\n";
						j = stringQueue.size();
					}
				}
			}
		}
		else if (currentKeyword == "man") {
			if (i + 1 < stringQueue.size()) {
				if (stringQueue.at(i + 1) == "man") {
					outputString.append(
					    "MAN:\n\nman [instruction]\n\nUse with [man], [echo]"
						", [for], [cat], [ls], [tail], [$], [=], [&&] and"
						"[||] to learn more about those instructions.\n");
				}
				if (stringQueue.at(i + 1) == "echo") {
					outputString.append(
						"ECHO:\necho [text]\n Use to write something.\n");
				}
				if (stringQueue.at(i + 1) == "for") {
					outputString.append(
						"FOR:\n\nfor [iterator] in [val1] [val2] ... ; do "
						"[instructions] ; done\n\nUse to execute "
						"a loop.\n");
				}
				if (stringQueue.at(i + 1) == "cat") {
					outputString.append(
						"CAT:\n\ncat [file1] OR cat [file1] [file2] ...\n\n"
						"Use to read one or multiple files.\n");
				}
				if (stringQueue.at(i + 1) == "ls") {
					outputString.append("LS:\n\nls\n\n"
						"Use to show all files in current directory.\n");
				}
				if (stringQueue.at(i + 1) == "tail") {
					outputString.append("TAIL:\n\ntail [file1] OR "
						"tail [file1] [file2] ...\n\nUse to read the"
						"ten last lines of one or multiple files.\n");
				}
				if (stringQueue.at(i + 1) == "$") {
					outputString.append("$:\n\n$[var]"
						"\n\nUse to get the value of a stored variable."
						"\nUse echo $[var] to show the current value."
						"\nSet a store variable using [=].\n");
				}
				if (stringQueue.at(i + 1) == "=") {
					outputString.append("=:\n\n [var]=[text]"
						"\n\nSets [text] to be corresponding to [var] when"
						" called with $[var].\n");
				}
				if (stringQueue.at(i + 1) == "&&") {
					outputString.append("&&:\n\n[instruction] && [instruction]"
						"\n\nExecutes the second instruction if the first one "
						"was sucessful.\n");
				}
				if (stringQueue.at(i + 1) == "||") {
					outputString.append("||:\n\n[instruction] || [instruction]"
						"\n\nExecutes the second instruction if the first one "
						"was unsucessful.\n");
				}
				i++;
			}
			else {
				outputString = "Missing an instruction.";
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