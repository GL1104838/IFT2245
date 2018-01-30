/* ch.cpp
auteur: Gabriel Lemyre/Kevin Belisle
date: 26-01-2018
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
#include <errno.h>

/*Un-Comment the line below to run the automated tests!*/
//#define TEST

using namespace std;

void requestInput();
void readInput(string);
void readKeywords();
void readEcho(string);
void forLoop(string, vector<string>);
bool execCommand(vector<string>);
void transformAllVariables();
string getVariable(string);
int countAmountInQueue(string);


#ifdef TEST
int main_test();
void reset_State(bool);

void assert_readInput();
void assert_readKeywords();
void assert_Commands();

bool testQueue = false;
vector<char*> linuxArgs;
#endif // TEST


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
#ifdef TEST
	return main_test();
#endif // TEST

	cout << "%% ";

	/* ¡REMPLIR-ICI! : Lire les commandes de l'utilisateur et les exécuter. */
	requestInput();

	cout << "Bye!\n";
	return 0;
}

bool execCommand(vector<string> args)
{
	//Transform vector<string> into const char** and add NULL at the end
#ifdef TEST
	linuxArgs.reserve(args.size() + 1);
#endif // TEST
	vector<char*> argc;
	argc.reserve(args.size() + 1);
	for (int i = 0; i < args.size(); i++)
	{
#ifdef TEST
		linuxArgs.push_back(const_cast<char*>(args[i].c_str()));
#endif // TEST
		argc.push_back(const_cast<char*>(args[i].c_str()));
	}
	//Args need to end with NULL terminator
	argc.push_back(NULL);
#ifdef TEST
	linuxArgs.push_back(NULL);
#endif // TEST

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
			return state.si_status == 1;
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

	transformAllVariables();
	readKeywords();
}

void forLoop(string iterator, vector<string> loopSet) {
	if (loopSet.size() > 0) {
		vector<string> stringQueueCopy = stringQueue;

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

		transformAllVariables();
		readKeywords();
		stringQueue = stringQueueCopy;
		loopSet.erase(loopSet.begin());
		forLoop(iterator, loopSet);
	}
}

string getVariable(string currentVarName) {
	//get the currentVarName value
	for (int i = 0; i < variables.size(); i++) {
		if (variables.at(i).at(0) == currentVarName) {
			return variables.at(i).at(1);
		}
	}
	return "$";
}

bool isIterator(string iterator, vector<string> iteratorList) {
	for (int i = 0; i < iteratorList.size(); i++) {
		if (iterator == iteratorList.at(i)) {
			return true;
		}
	}
	return false;
}

int countAmountInQueue(string word) {
	int count = 0;
	for (int i = 0; i < stringQueue.size(); i++) {
		if (word == stringQueue.at(i)) {
			count++;
		}
	}
	return count;
}

void transformAllVariables() {
	//Transform all the variables in the stringQueue to their full string form

	vector<string> iteratorList;

	for (int i = 0; i < stringQueue.size(); i++) {
		//Loop for the entire string queue
		string currentKeyword = stringQueue.at(i);
		string currentNewWord;
		string currentVariableName;
		bool hasDollarSign = false;
		bool readingVariable = false;

		if (currentKeyword == "for") {
			//excludes the iterator from the variable replacement
			if (i + 1 < stringQueue.size()) {
				iteratorList.push_back(stringQueue.at(i + 1));
			}
		}

		for (int j = 0; j < currentKeyword.length(); j++) {
			//Verify is there is a dollar sign in the current word
			if (currentKeyword.at(j) != '$') {
				if (hasDollarSign) {
					readingVariable = true;
					currentVariableName += currentKeyword.at(j);
				}
				else {
					currentNewWord += currentKeyword.at(j);
				}
			}
			else {
				//There is a dollar sign in the current keyword
				if (readingVariable) {
					//Stores variable value and search for new variable
					if (isIterator(currentVariableName, iteratorList)) {
						currentNewWord += "$" + currentVariableName;
					}
					else {
						//if it isn't part of the for loop iterator
						currentNewWord += getVariable(currentVariableName);
					}
					currentVariableName = "";
				}
				hasDollarSign = true;
			}
		}
		if (readingVariable) {
			//read the last variable
			if (isIterator(currentVariableName, iteratorList)) {
				currentNewWord += "$" + currentVariableName;
			}
			else {
				currentNewWord += getVariable(currentVariableName);
			}
		}
		//Assign the currentNewWord to the stringQueue
		stringQueue.at(i) = currentNewWord;
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
									stringQueue.begin() + j + 1);
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

					int forCount = countAmountInQueue("for");

					for (int j = 0; j < stringQueue.size(); j++) {
						if (addInstructions) {
							instructionsAfterLoop.push_back(stringQueue.at(j));
							stringQueue.erase(stringQueue.begin() + j);
							j--;
						}
						else if (stringQueue.at(j) == "done") {
							if (forCount == 0) {
								stringQueue.erase(stringQueue.begin() + j);
								j--;
								addInstructions = true;
							}
							forCount--;
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
				for (int j = i + 1; j < stringQueue.size(); j++) {
					if (stringQueue.at(j) == ";" || stringQueue.at(j) == "||"
						|| stringQueue.at(j) == "&&") {
						i = j - 1;
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
						i = j - 1;
						j = stringQueue.size();
						interruptedLoop = true;
					}
				}
				if (!interruptedLoop) {
					i = stringQueue.size();
				}
			}
		}
		else if (currentKeyword != ";") {
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

#ifdef TEST
int main_test()
{
	assert_readInput();

	assert_readKeywords();
	assert_Commands();
	cout << "Press enter to exit...";
	char c;
	do c = getchar(); while ((c != '\n') && (c != EOF));
	return 0;
}
void assert_readInput()
{
	testQueue = true;
	cout << "Testing readInput()\n";
	//Test queue
	//Check Empty Queue
	//Expected : queue.size = 0
	readInput("");
	if (stringQueue.size() == 0)
		cout << "  Empty Queue : Sucess\n";
	else
		cout << "  Empty Queue : Failed\n";
	reset_State(true);

	//Check Queue Size (no trailing spaces)
	readInput("test test test test test");
	if (stringQueue.size() == 5)
		cout << "  No Space Queue Size : Sucess\n";
	else
	{
		cout << "  No Space Queue Size : Failed. Expected : 5, Actual : ";
		cout << stringQueue.size() << "\n";
		for (int i = 0; i < stringQueue.size(); i++)
		{
			cout << "(" << i + 1 << ")" << stringQueue.at(i) << "/\n";
		}
	}
	reset_State(true);

	//Check Queue Size (with spaces)
	readInput("test  test   test   ");
	if (stringQueue.size() == 3)
		cout << "  Space Queue Size : Sucess\n";
	else
	{
		cout << "  Space Queue Size : Failed. Expected : 3, Actual : ";
		cout << stringQueue.size() << "\n";
		for (int i = 0; i < stringQueue.size(); i++)
		{
			cout << "  (" << i + 1 << ")" << stringQueue.at(i) << "/\n";
		}
	}
	reset_State(true);
	//Queue Content
	vector<string> compareQueue;
	compareQueue.push_back("echo");
	compareQueue.push_back("word1");
	compareQueue.push_back("word2");
	readInput("echo word1 word2");
	bool assert = true;
	for (int i = 0; i < stringQueue.size(); i++)
	{
		if (stringQueue.at(i) != compareQueue.at(i))
		{
			assert = false;
			cout << "  (" << i + 1 << ") ";
			cout << "Expected : " << compareQueue.at(i);
			cout << ", Actual" << stringQueue.at(i);
		}
	}
	if (assert) cout << "  Queue Content : Sucess\n";
	else cout << "  Queue Content : Failed\n";
	reset_State(true);
	testQueue = false;
}
void assert_Commands()
{
	//Linux Command
	string output = "Testing execCommands()\n";
	//We test if we give the OS the right arguments
	//We, manually, test if the fork()-ing and exec.
	//Empty Echo
	readInput("echo");
	if (linuxArgs.size() == 2
		&& strcmp("echo", linuxArgs.at(0)) == 0
		&& linuxArgs.at(1) == NULL)
		output += "  Empty Echo : Sucess.\n";
	else
		output += "  Empty Echo : Failed.\n";
	reset_State(true);
	//Echo without Variables
	readInput("echo bonjour");
	if (linuxArgs.size() == 3
		&& strcmp("echo", linuxArgs.at(0)) == 0
		&& strcmp("bonjour", linuxArgs.at(1)) == 0
		&& linuxArgs.at(2) == NULL)
		output += "  Echo without Variables : Sucess.\n";
	else
		output += "  Echo without Variables : Failed.\n";
	reset_State(true);
	//Echo with variables
	readInput("a=1");
	reset_State(false);
	readInput("echo $a");
	if (linuxArgs.size() == 3
		&& strcmp("echo", linuxArgs.at(0)) == 0
		&& strcmp("1", linuxArgs.at(1)) == 0
		&& linuxArgs.at(2) == NULL)
		output += "  Echo with Variables : Sucess.\n";
	else
		output += "  Echo with Variables : Failed.\n";
	reset_State(true);
	//Echo multiples arguments
	readInput("a=1");
	reset_State(false);
	readInput("echo Hello World Test 1234 $a");
	if (linuxArgs.size() == 7
		&& strcmp("echo", linuxArgs.at(0)) == 0
		&& strcmp("Hello", linuxArgs.at(1)) == 0
		&& strcmp("World", linuxArgs.at(2)) == 0
		&& strcmp("Test", linuxArgs.at(3)) == 0
		&& strcmp("1234", linuxArgs.at(4)) == 0
		&& strcmp("1", linuxArgs.at(5)) == 0
		&& linuxArgs.at(6) == NULL)
		output += "  Echo multiples arguments : Sucess.\n";
	else
		output += "  Echo multiples arguments : Failed.\n";
	reset_State(true);
	cout << "\n\n" << output;
}
void assert_readKeywords()
{
	//Variables
	cout << "Testing readKeywords()\n";
	//Set Variable
	readInput("a=1");
	reset_State(false);
	if (variables.size() == 1
		&& variables.at(0).at(0) == "a"
		&& variables.at(0).at(1) == "1")
		cout << "  Set Variable : Sucess.\n";
	else
		cout << "  Set Variable : Failed.\n";
	reset_State(true);
	//Get Variable
	readInput("a=test");
	reset_State(false);
	readInput("b=1234");
	reset_State(false);
	bool assert = false;
	if (variables.size() == 2)
	{
		for (int i = 0; i < 2; i++)
		{
			if (variables.at(i).at(0) == "b"
				&& variables.at(i).at(1) == "1234")
			{
				if (assert)
					assert = false;
				else
					assert = true;
			}
		}
	}
	if (assert)
		cout << "  Get Variable : Sucess.\n";
	else
		cout << "  Get Variable : Failed.\n";
	reset_State(true);

	//&&
	cout << "\n  AND Output :\n";
	readInput("echo abc && echo def");
	cout << "  AND Expected Output:\nabc\ndef\n\n";
	reset_State(true);

	//||
	cout << "  OR Output :\n";
	readInput("echo abc || echo def");
	cout << "  OR Expected Output:\nabc\n\n";
	reset_State(true);

	//variable concatenation
	readInput("a=1");
	reset_State(false);
	readInput("echo Hello$a");
	if (linuxArgs.size() == 3
		&& strcmp("echo", linuxArgs.at(0)) == 0
		&& strcmp("Hello1", linuxArgs.at(1)) == 0
		&& linuxArgs.at(2) == NULL)
		cout << "  Variable concatenation : Sucess.\n";
	else
		cout << "  Variable concatenation : Failed.\n";
	reset_State(true);



	//for
	readInput("for i in 1 2 3 ; do echo $i ; echo $i ; done");
	if (invalidInput)
		cout << "  For Syntax : Failed.\n";
	else
		cout << "  For Syntax : Sucess.\n";
	reset_State(true);
	//for Invalid Syntax
	readInput("for i 1 2 3 ; do a$i=test$i ; done");
	if (invalidInput)
		cout << "  For Invalid Syntax : Sucess.\n";
	else
		cout << "  For Invalid Syntax : Failed.\n";
	reset_State(true);
}
void reset_State(bool resetVariables)
{
	linuxArgs.clear();
	outputString = "";
	stringQueue.clear();
	invalidInput = false;
	if (resetVariables) variables.clear();
}
#endif // TEST
