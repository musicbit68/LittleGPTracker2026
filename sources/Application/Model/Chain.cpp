#include "Chain.h"
#include "System/System/System.h"
#include <stdlib.h>
#include <string.h>

Chain::Chain() {

	int size=CHAIN_COUNT*16 ;
	data_=(unsigned char *)SYS_MALLOC(size) ;
	memset(data_,0xFF,size) ;
	transpose_=(unsigned char *)SYS_MALLOC(size) ;
	memset(transpose_,0x00,size) ;
	for (int i=0;i<CHAIN_COUNT;i++) {
		isUsed_[i]=false ;
	}
	memset(names_,0,sizeof(names_)) ;
} ;

Chain::~Chain() {
	if (data_) SYS_FREE(data_) ;
	if (transpose_) SYS_FREE(transpose_) ;
};

void Chain::SetName(int i, const char *name) {
	strncpy(names_[i],name,sizeof(names_[i])-1) ;
	names_[i][sizeof(names_[i])-1]='\0' ;
} ;

unsigned short Chain::GetNext() {
	for (int i=0;i<CHAIN_COUNT;i++) {
		if (!isUsed_[i]) {
			isUsed_[i]=true ;
			return i ;
		}
	}
	return NO_MORE_CHAIN ;
} ;

void Chain::SetUsed(unsigned char c) {
	isUsed_[c]=true ;
}

void Chain::ClearAllocation() {

	for (int i=0;i<CHAIN_COUNT;i++) {
		isUsed_[i]=false ;
	}
}
