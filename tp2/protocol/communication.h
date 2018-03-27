#ifndef COMMUNICATION_H
#define COMMUNICATION_H

typedef enum communication_type {
	BEG,
	PRO,
	END,
	INI,
	REQ,
	CLO,
	ACK,
	ERR,
	WAIT
};

typedef struct communication_data {
	enum communication_type communication_type;
	int * args;
	unsigned int args_count;
};

void read_communication(FILE *, FILE *, struct communication_data *);
void write_communication(FILE *, struct communication_data *);
bool setup_communication_data(FILE *, FILE *, struct communication_data *);

#endif