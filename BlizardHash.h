#ifndef _BLIZARD_HASH_H_
#define _BLIZARD_HASH_H_


#include <stdlib.h>
#include <stdio.h>
#include <map>


#include "windows.h"


static const int HashSize[] = {17, 37, 79, 163, 331,
673,           1361,        2729,       5471,         10949,
21911,          43853,      87719,      175447,      350899,
701819,         1403641,    2807303,     5614657,     11229331,
22458671,       44917381,    89834777,    179669557,   359339171,
718678369,      1437356741,  2147483647 };


/*
*
*
*
*/
template<class T>
class BlizardHash
{
public:
	static int const INIT_HASH_SIZE = 8;

	typedef struct tagHashNode
	{
		unsigned int hashFirst;
		unsigned int hashSecond;
		char*        key;
		T            value;

		tagHashNode():hashFirst(0), hashSecond(0), 
			          key(0), value()
		{

		}

		tagHashNode( const char* _key, const T& _value ): hashFirst(0),
			        hashSecond(0),  value(_value)
		{
			SetKey( _key );
		}

		tagHashNode( unsigned int hash1, unsigned int hash2,const char* _key, const T& _value ):
		    hashFirst(hash1),
			hashSecond(hash2),  
			value(_value)
		{
			SetKey( _key );
		}

	    void SetKey( const char* _key )
		{
			size_t len = strlen(_key) + 1;
			key = new char[len];
			strncpy( key, _key, len - 1);
			key[len - 1] = '\0';
		}

		~tagHashNode()
		{
			delete [] key;
			key = 0;
		}


	}HashNode, *pHashNode;


	/*
	*
	*
	*/
	BlizardHash():m_cryptTable(), m_curSize(INIT_HASH_SIZE),
		          m_tableSize(0), m_tableUsed(0),
				  m_hashTable(0)
	{
		Init();
	}

	/*
	*
	*
	*/
	~BlizardHash()
	{
		Clear();
	}

	/*
	*
	*
	*/
	void Clear()
	{
		Clear( m_hashTable, m_tableSize );

		m_tableSize = 0;
		m_tableUsed = 0;
	}

	/*
	*
	*
	*/
	void Insert( const char* key, const T& value )
	{
		Rehash();

		unsigned int pos = GetHashTablePos( key );
		if( -1 == pos )
		{
			if( Insert( m_hashTable, m_tableSize, key, value ) )
			{
				m_tableUsed++;
			}
		}
	}


	/*
	*
	*
	*/
	T* Find( const char* key )
	{
		unsigned int pos = GetHashTablePos( key );
		if( pos != -1 )
		{
			if( KeyCompare( key, m_hashTable[pos]->key ) )
				return &m_hashTable[pos]->value;
		}
		
		return NULL;
	}


	/*
	*
	*
	*/
	void Delete( const char* key )
	{
		unsigned int pos = GetHashTablePos( key );
		if( pos != -1 )
		{
			if( KeyCompare( key, m_hashTable[pos]->key ) )
			{
				delete m_hashTable[pos];
				m_hashTable[pos] = 0;
				m_tableUsed--;
			}
		}
	}



protected:

	void Clear( pHashNode* hashTable, unsigned int size )
	{
		for( unsigned int i = 0; i < size; i++ )
		{
			if( hashTable[i] )
			{
				delete hashTable[i];
				hashTable[i] = 0;
			}
		}

		delete [] hashTable;
		hashTable = 0;
	}

	/*
	*
	*
	*/
	bool KeyCompare( const char* keyFirst, const char* keySecond )
	{
		size_t len1 = strlen(keyFirst);
		size_t len2 = strlen(keySecond);
		return len1 == len2 && !strcmp( keyFirst, keySecond );
	}




	/*
	*
	*
	*/
	unsigned long HashFunctor( const char* inputStr, int hashType )
	{
		unsigned char *key = (unsigned char *)inputStr;	
		unsigned long seed1 = 0x7FED7FED, seed2 = 0xEEEEEEEE;

		int ch;
		while(*key != 0)	
		{	
			ch = toupper(*key++);		
			seed1 = m_cryptTable[(hashType << 8) + ch] ^ (seed1 + seed2);		
			seed2 = ch + seed1 + seed2 + (seed2 << 5) + 3;		
		}

		return seed1;
	}

	/*
	*
	*
	*/
	void PrepareCryptTable()
	{
		unsigned long seed = 0x00100001, index1 = 0, index2 = 0;
		unsigned long i = 0;
		for( index1 = 0; index1 < 0x100; index1++ )
		{
			for( index2 = index1, i = 0; i < 5; i++, index2 += 0x100 )
			{

				unsigned long temp1, temp2;
				seed = (seed * 125 + 3) % 0x2AAAAB;
				temp1 = (seed & 0xFFFF) << 0x10;
				seed = (seed * 125 + 3) % 0x2AAAAB;
				temp2 = (seed & 0xFFFF);

				m_cryptTable[index2] = ( temp1 | temp2 );
			}

		}
	}

	/*
	*
	*
	*/
	unsigned int GetHashTablePos( const char *inputStr )
	{
		const unsigned long HASH_OFFSET = 0, HASH_A = 1, HASH_B = 2;
		unsigned long nHash = HashFunctor(inputStr, HASH_OFFSET);
		unsigned long nHashA = HashFunctor(inputStr, HASH_A);
		unsigned long nHashB = HashFunctor(inputStr, HASH_B);

		unsigned long nHashStart = nHash % m_tableSize,
		nHashPos = nHashStart;

		while( m_hashTable[nHashPos] )
		{
			if( m_hashTable[nHashPos]->hashFirst == nHashA && m_hashTable[nHashPos]->hashSecond == nHashB )
			{
				return nHashPos;
			}
			else
			{
				nHashPos = (nHashPos + 1) % m_tableSize;
			}		

			if( nHashPos == nHashStart )
				break;

		}

		return -1;
	}

	/*
	*
	*
	*/
	void Rehash()
	{
		if( m_tableUsed >= m_tableSize )
		{
			size_t allSize = sizeof(HashSize)/sizeof( HashSize[0] );
			if( m_curSize < allSize - 1 )
			{
				m_curSize++;
				pHashNode* newTable = new pHashNode[HashSize[m_curSize]];
				memset( newTable, 0x00, sizeof(pHashNode) * HashSize[m_curSize] );
				for( int i = 0; i < m_tableSize; i++ )
				{
					Insert( newTable, HashSize[m_curSize], m_hashTable[i]->key, m_hashTable[i]->value );
				}

				Clear( m_hashTable, m_tableSize );


				m_hashTable = newTable;
				m_tableSize = HashSize[m_curSize];
			}
		}

	}

	/*
	*
	*
	*/
	bool Insert( pHashNode* hashTable, unsigned int tableSize, const char* key, const T& value )
	{
		const unsigned long HASH_OFFSET = 0, HASH_A = 1, HASH_B = 2;

		unsigned long nHash = HashFunctor(key, HASH_OFFSET);
		unsigned long nHashA = HashFunctor(key, HASH_A);
		unsigned long nHashB = HashFunctor(key, HASH_B);
		unsigned long nHashStart = nHash % tableSize, nHashPos = nHashStart;

		while( hashTable[nHashPos] )
		{
			nHashPos = (nHashPos + 1) % tableSize;	
			if( nHashPos == nHashStart )			
			{
				unsigned long pos = nHash % tableSize;
				pHashNode node = new HashNode( nHashA, nHashB, key, value );
				assert( node );
				hashTable[pos] = node;

				return true;
			}
		
		}

		if( !hashTable[nHashPos] )
		{
			pHashNode node = new HashNode( nHashA, nHashB, key, value );
			assert( node );
			hashTable[nHashPos] = node;

			return true;
		}

		return false;
	}


	/*
	*
	*
	*/
	void Init()
	{
		PrepareCryptTable();

		size_t allSize = sizeof(HashSize)/sizeof( HashSize[0] );
		if( m_curSize < allSize - 1 )
		{
			m_tableSize = HashSize[m_curSize];
			m_hashTable = new pHashNode[m_tableSize];
			memset( m_hashTable, 0x00, sizeof(pHashNode)*m_tableSize );
			
		}
		else
		{
			m_curSize = INIT_HASH_SIZE;
			m_tableSize = HashSize[m_curSize];
			m_hashTable = new pHashNode[m_tableSize];
			memset( m_hashTable, 0x00, sizeof(pHashNode)*m_tableSize );
			
		}
		
	}

private:

	unsigned long m_cryptTable[0x500];

	size_t         m_curSize;

	unsigned long  m_tableSize;   

	unsigned long  m_tableUsed;

	pHashNode*     m_hashTable;

	

};



/*
* Test hash table
*
*/
void TestBlizardHashTable()
{
	unsigned long start = GetTickCount();

	BlizardHash<int> hashTable;
	const int Len = 500000;
	for( int i = 0; i < Len; i++ )
	{
		char key[16] = {0};
		sprintf(key, "%s_%d", "china", i );

		hashTable.Insert( key, i );
	}


	for( int i = 0; i < Len; i++ )
	{
		char key[16] = {0};
		sprintf(key, "%s_%d", "china", i );

		if( i > 0 && !(i % 50) )
		{
			hashTable.Delete( key );
			assert( !hashTable.Find( key ) );
		}
		else
		{
			assert(i == *hashTable.Find( key));
		}


	}

	unsigned long interval = GetTickCount() - start;
	printf(" hash table consume time is %d \n", interval );
}


/*
* Test STL map
*
*/
void TestBlizardSTLMap()
{
	unsigned long start = GetTickCount();

	std::map<std::string, int > strMap;
	const int Len = 500000;
	for( int i = 0; i < Len; i++ )
	{
		char key[16] = {0};
		sprintf(key, "%s_%d", "china", i );
		std::string keyStr(key);

		strMap.insert( std::make_pair(keyStr, i )) ;
	}

	std::map<std::string, int >::iterator iter = strMap.begin();
	for( int i = 0; i < Len; i++ )
	{
		char key[16] = {0};
		sprintf(key, "%s_%d", "china", i );
		std::string keyStr(key);

		if( i > 0 && !(i % 50) )
		{
			strMap.erase( key );
			assert( strMap.find( key ) == strMap.end() );
		}
		else
		{
			iter = strMap.find( keyStr );
			assert( iter->second == i );
		}

	}



	unsigned long interval = GetTickCount() - start;
	printf(" STL map consume time is %d \n", interval );
}


/*
* Test suite and compare performance
*
*/
void TestSuiteBlizardHash()
{
	TestBlizardHashTable();
	TestBlizardSTLMap();

}


#endif 