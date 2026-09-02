#ifndef _CHAIN_H_
#define _CHAIN_H_

#define CHAIN_COUNT 0xFF
#define NO_MORE_CHAIN 0x100

class Chain {
public:
	Chain() ;
	~Chain() ;
	unsigned short GetNext() ;
	bool IsUsed(unsigned char i) { return isUsed_[i] ; } ;
	void SetUsed(unsigned char c) ;
	void ClearAllocation() ;
	const char *GetName(int i) { return names_[i]; };
	void SetName(int i, const char *name);

	unsigned char *data_ ;
	unsigned char *transpose_ ;
	char names_[CHAIN_COUNT][13] ;

	
private:
	bool isUsed_[CHAIN_COUNT] ;

} ;

#endif
