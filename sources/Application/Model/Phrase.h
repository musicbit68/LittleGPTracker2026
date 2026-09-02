#ifndef _PHRASE_H_
#define _PHRASE_H_

#include "Foundation/Types/Types.h"
#define PHRASE_COUNT 0xFF
#define NO_MORE_PHRASE 0x100

class Phrase {
public:
	Phrase() ;
	~Phrase() ;
	unsigned short GetNext() ;
	bool IsUsed(uchar i) { return isUsed_[i] ; } ;
	void SetUsed(uchar c) ;
	void ClearAllocation() ;
	const char *GetName(int i) { return names_[i]; };
	void SetName(int i, const char *name);

	uchar *note_ ;
	uchar *instr_ ;
	FourCC *cmd1_ ;
	ushort *param1_ ;
	FourCC *cmd2_ ;
	ushort *param2_ ;
	char names_[PHRASE_COUNT][13] ;
	
private:
	bool isUsed_[PHRASE_COUNT] ;

} ;

#endif
