/* ch.c
auteur: Gabriel Lemyre/Kevin Belisle
date: 26-01-2018
problèmes connus: 
-Currently can't assign a variable and use it on the same line
-Not formatted for 80 columns
*/

#define string char*
#define MAX_INPUT_SIZE 257 // 255 +  for "\n\0"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

void requestInput();
void readInput(string);
void readKeywords(int, int);
void cleanStringQueue();
void readEcho(string);
void forLoop(string, char[MAX_INPUT_SIZE][MAX_INPUT_SIZE], int, int, int, int);
void transformAllVariables(int, int);
string getVariable(string);
int countAmountInQueue(string);

//Contains parsed words
char stringQueue[MAX_INPUT_SIZE][MAX_INPUT_SIZE] = { { '\0' } };
int stringQueueCounter = 0;
int stringQueueForcedPosition = 0;
char stringOriginalQueue[MAX_INPUT_SIZE][MAX_INPUT_SIZE] = { { '\0' } };

//Contains the instructions after the "done" input in a for loop
char instructionsAfterLoop[MAX_INPUT_SIZE][MAX_INPUT_SIZE] = { { '\0' } };

//Contains a maximum of 1000 variables
//variables[0] is the variables name, 1 is the value
//variables[0][x] is to get the string
//variables[0][x][y] is to get a specific char
char variables[2][1000][MAX_INPUT_SIZE] = { { { '\0' } } };
int variablesCount = 0;


//Will be shown to the user
string outputString;

bool invalidInput = false;

int main()
{
	printf("%% ");

	/* ¡REMPLIR-ICI! : Lire les commandes de l'utilisateur et les exécuter. */
	requestInput();

	printf("Bye!\n");
	return 0;
}

void requestInput() {
	//Called after every instruction

	char inputString[MAX_INPUT_SIZE];
	printf("Shell> ");

	fgets(inputString, sizeof(inputString), stdin);

	//removes the last character from the input string
	char newInputString[MAX_INPUT_SIZE] = { '\0' };
	for (int i = 0; i < strlen(inputString) - 1; i++) {
		newInputString[i] = inputString[i];
	}

	outputString = "\ntemporary output string\n";

	readInput(newInputString);

	if (invalidInput) {
		outputString = "Invalid input error.\n";
	}
	//outputString = "temporary output string\n\n";
	printf(outputString);

	//Variable cleanup
	outputString = "";
	cleanStringQueue();
	stringQueueCounter = 0;
	stringQueueForcedPosition = 0;
	invalidInput = false;

	requestInput();
}

void readInput(string inputString) {
	//Reads the input

	//Read the words and put them in the stringQueue
	int stringQueuePos = 0;
	for (int i = 0; i < strlen(inputString); i++) {
		if (!(inputString[i] == ' ')) {
			//if the char is not an empty space

			stringQueue[stringQueueCounter][stringQueuePos] = inputString[i];
			stringOriginalQueue[stringQueueCounter][stringQueuePos] = inputString[i];
			stringQueuePos++;
		}
		else {
			//if the char is an empty space
			if (strlen(stringQueue[stringQueueCounter]) != 0) {
				stringQueueCounter++;
				stringQueuePos = 0;
			}
		}
	}
	transformAllVariables(0, stringQueueCounter);
	readKeywords(stringQueueForcedPosition, stringQueueCounter);
}

void cleanStringQueue() {
	for (int i = 0; i < MAX_INPUT_SIZE; i++) {
		for (int j = 0; j < MAX_INPUT_SIZE; j++) {
			stringQueue[i][j] = '\0';
		}
	}
}

void forLoop(string iterator, char loopSet[MAX_INPUT_SIZE][MAX_INPUT_SIZE],
	int loopSetPosition, int loopSetSize, int stringQueueStart,
	int stringQueueEnd) {
	if (loopSetPosition < loopSetSize) {

		bool variableIsDefined = false;
		for (int i = 0; i < variablesCount; i++) {
			if (!strcmp(iterator, variables[0][i])) {
				variableIsDefined = true;

				//clear the current value
				int variableStringSize = strlen(variables[1][i]);
				for (int j = 0; j < variableStringSize; j++) {
					variables[1][i][j] = '\0';
				}

				for (int j = 0; j < strlen(loopSet[loopSetPosition]); j++) {
					variables[1][i][j] = loopSet[loopSetPosition][j];
				}
			}
		}
		if (!variableIsDefined) {
			//Create a new variable
			for (int i = 0; i < strlen(iterator); i++) {
				variables[0][variablesCount][i] = iterator[i];
			}
			for (int i = 0; i < strlen(loopSet[loopSetPosition]); i++) {
				variables[1][variablesCount][i] = loopSet[loopSetPosition][i];
			}
			variablesCount++;
		}

		//backs up the queue before transforming the variables
		char oldStringQueue[MAX_INPUT_SIZE][MAX_INPUT_SIZE] = { { '\0' } };
		for (int i = 0; i < stringQueueCounter; i++) {
			for (int j = 0; j < strlen(stringQueue[i]); j++) {
				oldStringQueue[i][j] = stringQueue[i][j];
			}
		}

		//loads the original queue
		for (int i = 0; i < stringQueueCounter; i++) {
			for (int j = 0; j < strlen(stringQueue[i]); j++) {
				stringQueue[i][j] = stringOriginalQueue[i][j];
			}
		}

		transformAllVariables(stringQueueStart, stringQueueEnd);
		readKeywords(stringQueueStart, stringQueueEnd);

		//erases the current stringQueue
		cleanStringQueue();

		//loads the backup into stringQueue
		for (int i = 0; i < stringQueueCounter; i++) {
			for (int j = 0; j < strlen(oldStringQueue[i]); j++) {
				stringQueue[i][j] = oldStringQueue[i][j];
			}
		}


		forLoop(iterator, loopSet, loopSetPosition + 1, loopSetSize, stringQueueStart, stringQueueEnd);
	}
}

string getVariable(string currentVarName) {
	//get the currentVarName value
	for (int i = 0; i < variablesCount; i++) {
		if (!strcmp(variables[0][i], currentVarName)) {
			return variables[1][i];
		}
	}
	return "$";
}

bool isIterator(string iterator, char iteratorList[MAX_INPUT_SIZE][MAX_INPUT_SIZE], int iteratorListCount) {
	for (int i = 0; i < iteratorListCount; i++) {
		if (!strcmp(iterator, iteratorList[i])) {
			return true;
		}
	}
	return false;
}

int countAmountInQueue(string word) {
	int count = 0;
	for (int i = 0; i < stringQueueCounter; i++) {
		if (!strcmp(word, stringQueue[i])) {
			count++;
		}
	}
	return count;
}

void transformAllVariables(int startPoint, int endPoint) {
	//Transform all the variables in the stringQueue to their full string form

	char iteratorList[MAX_INPUT_SIZE][MAX_INPUT_SIZE] = { { '\0' } };
	int iteratorListCount = 0;

	for (int i = startPoint; i <= endPoint; i++) {
		//Loop for the entire string queue
		char currentNewWord[MAX_INPUT_SIZE] = { '\0' };
		int currentNewWordPos = 0;

		char currentVariableName[MAX_INPUT_SIZE] = { '\0' };
		int currentVariablePos = 0;

		bool hasDollarSign = false;
		bool readingVariable = false;

		if (!strcmp("for", stringQueue[i])) {
			//excludes the iterator from the variable replacement
			if (i + 1 < endPoint) {
				for (int j = 0; j < strlen(stringQueue[i + 1]); j++) {
					iteratorList[iteratorListCount][j] = stringQueue[i + 1][j];
				}
				iteratorListCount++;
			}
		}

		for (int j = 0; j < strlen(stringQueue[i]); j++) {
			//Verify is there is a dollar sign in the current word
			if (stringQueue[i][j] != '$') {
				if (hasDollarSign) {
					readingVariable = true;
					currentVariableName[currentVariablePos] = stringQueue[i][j];
					currentVariablePos++;
				}
				else {
					currentNewWord[currentNewWordPos] = stringQueue[i][j];
					currentNewWordPos++;
				}
			}
			else {
				//There is a dollar sign in the current keyword
				if (readingVariable) {
					//Stores variable value and search for new variable
					if (isIterator(currentVariableName, iteratorList, iteratorListCount)) {
						currentNewWord[currentNewWordPos] = '$';
						currentNewWordPos++;
						for (int k = 0; k < strlen(currentVariableName); k++) {
							currentNewWord[currentNewWordPos] = currentVariableName[k];
							currentNewWordPos++;
						}
					}
					else {
						//if it isn't part of the for loop iterator
						for (int k = 0; k < strlen(getVariable(currentVariableName)); k++) {
							currentNewWord[currentNewWordPos] = getVariable(currentVariableName)[k];
							currentNewWordPos++;
						}
					}
					int currentVarNameLength = strlen(currentVariableName);
					for (int k = 0; k < currentVarNameLength; k++) {
						//clean currentVariableName
						currentVariableName[k] = '\0';
						currentVariablePos = 0;
					}
				}
				hasDollarSign = true;
			}
		}
		if (readingVariable) {
			//read the last variable
			if (isIterator(currentVariableName, iteratorList, iteratorListCount)) {
				currentNewWord[currentNewWordPos] = '$';
				currentNewWordPos++;
				for (int k = 0; k < strlen(currentVariableName); k++) {
					currentNewWord[currentNewWordPos] = currentVariableName[k];
					currentNewWordPos++;
				}
			}
			else {
				for (int k = 0; k < strlen(getVariable(currentVariableName)); k++) {
					currentNewWord[currentNewWordPos] = getVariable(currentVariableName)[k];
					currentNewWordPos++;
				}
			}
		}

		//Assign the currentNewWord to the stringQueue
		int stringQueueWordLength = strlen(stringQueue[i]);

		for (int j = 0; j < stringQueueWordLength; j++) {
			//Clean the current string input
			stringQueue[i][j] = '\0';
		}

		int stringCurrentWordLength = strlen(currentNewWord);
		for (int j = 0; j < stringCurrentWordLength; j++) {
			stringQueue[i][j] = currentNewWord[j];
		}
	}
}

void readKeywords(int queuePosition, int queueEnd) {
	bool commandFailed = false;
	//Reads the keywords from the stringQueue
	for (int i = queuePosition; i <= queueEnd; i++) {

		/*if (currentKeyword == "echo" ||
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
		}*/
		/*else if (currentKeyword[0] == '$') {
		//if the current keyword is variable command
		string stringVariable =
		currentKeyword.substr(1, currentKeyword.size() - 1);
		for (int j = 0; j < variables.size(); j++) {
		if (stringVariable == variables.at(j).at(0)) {
		stringQueue.at(i) = variables.at(j).at(1);
		i--;
		}
		}
		}*/
		if (!strcmp("echo", stringQueue[i])) {
			//Temporary measure for testing purposes
			printf("\n");
			for (int j = i + 1; j <= stringQueueCounter; j++) {
				if (!strcmp(";", stringQueue[j]) || !strcmp("||", stringQueue[j])
					|| !strcmp("&&", stringQueue[j])
					|| !strcmp("$", stringQueue[j])) {
					i = j-1;
					j = stringQueueCounter;
				}
				else {
					printf(stringQueue[j]);
					printf(" ");
					i++;
				}
			}
		}
		else if (!strcmp("for", stringQueue[i])) {
			//if the current keyword is for
			char loopSet[MAX_INPUT_SIZE][MAX_INPUT_SIZE] = { { '\0' } };
			int loopSetCounter = 0;
			string iterator;

			int loopStart = 0;
			int loopEnd = 0;

			if (i + 1 < stringQueueCounter) {
				//Initialize the iterator
				iterator = stringQueue[i + 1];

				if (i + 2 < stringQueueCounter) {
					if (!strcmp("in", stringQueue[i + 2])) {
						for (int j = i + 3; j < stringQueueCounter; j++) {
							if (!strcmp(";", stringQueue[j])) {
								//end of the loopset

								stringQueueForcedPosition = j + 1;
								j = stringQueueCounter;
								// exit the for loop
							}
							else {
								//This copies the string into the loopSet
								for (int k = 0; k < strlen(stringQueue[j]); k++) {
									loopSet[j - i - 3][k] = stringQueue[j][k];
								}
								loopSetCounter++;
							}
						}
					}
					else {
						outputString = "Missing 'in' statement\n";
					}
				}
			}
			if (strcmp("\0", stringQueue[stringQueueForcedPosition]) != 0) {
				//if there is a do statement
				if (!strcmp("do", stringQueue[stringQueueForcedPosition])) {
					stringQueueForcedPosition++;
					loopStart = stringQueueForcedPosition;
					int forCount = countAmountInQueue("for") - 1;
					bool doneFound = false;

					for (int j = stringQueueForcedPosition; j <= stringQueueCounter; j++) {
						if (!strcmp("done", stringQueue[j])) {
							if (forCount == 0) {
								doneFound = true;
								loopEnd = j - 1;
								stringQueueForcedPosition = j + 1;

								forLoop(iterator, loopSet, 0, loopSetCounter, loopStart, loopEnd);

								for (int k = 0; k < variablesCount; k++) {
									if (!strcmp(variables[0][k], iterator)) {
										int varKLength = strlen(variables[1][k]);
										for (int m = 0; m < varKLength; m++) {
											variables[1][k][m] = '\0';
										}
										//exit loop
										k = variablesCount;
									}
								}

								j = stringQueueCounter;
								//end the for j loop
							}
							forCount--;
						}
					}
					if (!doneFound) {
						outputString = "Missing 'done' statement\n";
					}
				}
				else {
					outputString = "Missing 'do' statement\n";
				}
			}
		}

		else if (!strcmp("||", stringQueue[i])) {
			//Defines if the following text is to be executed
			if (!commandFailed) {
				bool interruptedLoop = false;
				for (int j = i + 1; j < stringQueueCounter; j++) {
					if (!strcmp(";", stringQueue[j]) || !strcmp("||", stringQueue[j])
						|| !strcmp("&&", stringQueue[j])) {
						i = j - 1;

						//interrupt loop
						j = stringQueueCounter;
						interruptedLoop = true;
					}
				}
				if (!interruptedLoop) {
					i = stringQueueCounter;
				}
			}
			commandFailed = false;
		}
		else if (!strcmp("&&", stringQueue[i])) {
			if (commandFailed) {
				bool interruptedLoop = false;
				for (int j = i + 1; j < stringQueueCounter; j++) {
					if (!strcmp(";", stringQueue[j]) || !strcmp("||", stringQueue[j])
						|| !strcmp("&&", stringQueue[j])) {
						i = j - 1;

						//interrupt loop
						j = stringQueueCounter;
						interruptedLoop = true;
					}
				}
				if (!interruptedLoop) {
					i = stringQueueCounter;
				}
			}
		}
		else if (!strcmp("\0", stringQueue[0])) {
			i = queueEnd + 1;
		}
		else if (strcmp(";", stringQueue[i])) {
			//Verify if it is a variable assignment
			bool isVariableAssigment = false;
			int equalSignPos;
			for (int j = 0; j < strlen(stringQueue[i]); j++) {
				if (stringQueue[i][j] == '=') {
					isVariableAssigment = true;
					equalSignPos = j;
					j = strlen(stringQueue[i]);
				}
			}
			if (isVariableAssigment) {
				//Assign the variable or modify it
				bool modifiedVariable = false;
				char variableName[MAX_INPUT_SIZE] = { '\0' };
				char variableValue[MAX_INPUT_SIZE] = { '\0' };

				for (int j = 0; j < equalSignPos; j++) {
					//Sets the variableName
					variableName[j] = stringQueue[i][j];
				}
				for (int j = equalSignPos + 1; j < strlen(stringQueue[i]); j++) {
					//Sets the variableValue
					variableValue[j - equalSignPos - 1] = stringQueue[i][j];
				}

				for (int j = 0; j < variablesCount; j++) {
					//Search if the variable exists
					if (!strcmp(variables[0][j], variableName)) {
						//It exists, modify it
						modifiedVariable = true;

						for (int k = 0; k < MAX_INPUT_SIZE; k++) {
							//resets the variable value
							variables[1][j][k] = '\0';
						}

						//Sets the new variable value
						for (int k = 0; k < strlen(variableValue); k++) {
							variables[1][j][k] = variableValue[k];
						}
					}
				}
				if (!modifiedVariable) {
					//assign the variable
					for (int j = 0; j < strlen(variableName); j++) {
						variables[0][variablesCount][j] = variableName[j];
					}
					for (int j = 0; j < strlen(variableValue); j++) {
						variables[1][variablesCount][j] = variableValue[j];
					}
					variablesCount++;
				}
			}
			else {
				//The input is invalid
				invalidInput = true;
				i = stringQueueCounter;
			}
		}
		else if (!strcmp("$", stringQueue[i])) {
			//it is an unassigned variable call
		}
		else if (!strcmp(";", stringQueue[i])) {
			commandFailed = false;
		}

		if (stringQueueForcedPosition > i
			&& stringQueueForcedPosition < queueEnd + 1)
		{
			//If we need to force a position unto i
			i = stringQueueForcedPosition;
		}
	}
}