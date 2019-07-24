/*---------------------------------------------------------------

네트워크 패킷용 클래스.
간편하게 패킷에 순서대로 데이타를 In, Out 한다.

- 사용법.

CPacket cPacket;

넣기.
cPacket << 40030;	or	cPacket << iValue;	(int 넣기)
cPacket << 3;		or	cPacket << byValue;	(BYTE 넣기)
cPacket << 1.4;	    or	cPacket << fValue;	(float 넣기)

빼기.
cPacket >> iValue;	(int 빼기)
cPacket >> byValue;	(BYTE 빼기)
cPacket >> fValue;	(float 빼기)

!.	삽입되는 데이타 FIFO 순서로 관리된다.
큐가 아니므로, 넣기(<<).빼기(>>) 를 혼합해서 사용하면 안된다.

16.11.21
- TLS 버전

- 시작 과정
1. CPacket의 CMemoryPool_TLS<CPacket> 생성 
2. CMemoryPool_TLS의 CMemoryPool_LF<CChunkBlock<TLS_DATA>> 생성
3. CMemoryPool_LF의 Alloc을 통해 CChunkBlock 생성자 호출
4. CMemoryPool_TLS의 Constructor()를 통해 CPacket 생성자 호출

- 종료 과정
1. CMemoryPool_TLS<CPacket> Delete 처리하면, ~CMemoryPool_TLS() 호출
2. CMemoryPool_TLS에서 CMemoryPool_LF를 Delete 처리하면, ~CMemoryPool_LF() 호출
3. CMemoryPool_LF에서 생성된 노드를 Delete 처리하면, ~CChunkBlock() 호출
----------------------------------------------------------------*/
#pragma once

#include <WinSock2.h>

#include "__NOH.h"

// tls
//#include "MemoryPool_TLS.h"
// lf
//#include "MemoryPool_LF.h"
// no_lf
//#include "MemoryPool.h"

namespace NOH
{
    struct exception_PacketOut
    {
	    explicit exception_PacketOut(int iSize) : _RequestOutSize(iSize) {};
	    int _RequestOutSize;
    };

    struct exception_PacketIn
    {
	    explicit exception_PacketIn(int iSize) : _RequestInSize(iSize) {};
	    int _RequestInSize;
    };

    template<class DATA> class CMemoryPool_TLS;
	class CPacket
	{
	public:
        typedef struct st_NET_HEADER
        {
	        char			cCode;
	        unsigned short	sLen;
	        char			cRandCode;
	        unsigned char	ucCheckSum;

            st_NET_HEADER() : cCode(0), sLen(0), cRandCode(0), ucCheckSum(0) {}
        } NET_HEADER;

		//-----------------------------------------------------------------------------------------
		// 생성자, 파괴자.
		//
		// 사용자가 사이즈를 지정할 경우 사용되는 생성자
		//-----------------------------------------------------------------------------------------
        explicit CPacket(const int iHeaderSize = static_cast<int>( PACKET::HEADER_MAX_SIZE ));
		explicit CPacket(const int iBufferSize, const int iHeaderSize = static_cast<int>( PACKET::HEADER_MAX_SIZE ));
		virtual	~CPacket();

		//-----------------------------------------------------------------------------------------
		// Param : void
		// Return: void
		//
		// 패킷 초기화.
		//
		// 메모리 할당을 여기서 하므로, 함부로 호출하면 안된다. 
		//-----------------------------------------------------------------------------------------
		void				Initial(void);

		//-----------------------------------------------------------------------------------------
		// Param :  void
		// Return: void
		//
		// 패킷 파괴
		//-----------------------------------------------------------------------------------------
		void				Release(void);

		//-----------------------------------------------------------------------------------------
		// Param : void
		// Return: CPacket *
		//
		// 패킷 청소
		//-----------------------------------------------------------------------------------------
		CPacket *			Clear(void);

		//-----------------------------------------------------------------------------------------
		// Param : void
		// Return: int (패킷 버퍼 사이즈)
		//
		// 총 버퍼 사이즈 얻기
		//-----------------------------------------------------------------------------------------
		int					GetBufferSize(void) { return m_iBufferSize; }

		//-----------------------------------------------------------------------------------------
		// Param : int (버퍼 사이즈)
		// Return: int (데이터 사이즈)
		//
		// 사용중인 데이터 사이즈 얻기
		//
		// Header 사이즈를 제외한 실제 payload 사이즈
		//-----------------------------------------------------------------------------------------
		int					GetPayloadSize(void) { return m_iUsingSize; }

		//-----------------------------------------------------------------------------------------
		// Param : void
		// Return: int (패킷 사이즈)
		//
		// 사용중인 패킷 사이즈 얻기
		//
		// Header 사이즈가 포함된 실제 패킷 사이즈
		//-----------------------------------------------------------------------------------------
		int					GetPacketSize(void) { return m_iUsingSize + m_iHeaderSize; }

		//-----------------------------------------------------------------------------------------
		// Param : void
		// Return: int (패킷 사이즈)
		//
		// (커스텀) 사용중인 패킷 사이즈 얻기
		//
		// 인자로 넘어온 Header 사이즈가 포함된 실제 패킷 사이즈
		//-----------------------------------------------------------------------------------------
		int					GetPacketSize_CustomHeader(void) { return m_iUsingSize + m_iHeaderSize; }

		//-----------------------------------------------------------------------------------------
		// Param : void
		// Return: char * (Header 버퍼 포인터)
		//
		// 시작 버퍼 포인터 얻기
		//-----------------------------------------------------------------------------------------
		char				*GetBufferPtr(void) { return m_cpBuffer; }

		//-----------------------------------------------------------------------------------------
		// Param : void
		// Return: char * (Header 버퍼 포인터)
		//
		// Header 버퍼 포인터 얻기
		//-----------------------------------------------------------------------------------------
		char				*GetHeaderBufferPtr(void) { return m_cpBuffer + (static_cast<int>( PACKET::HEADER_MAX_SIZE ) - static_cast<int>( PACKET::HEADER_DEFAULT_SIZE ) ); }

		//-----------------------------------------------------------------------------------------
		// Param : void
		// Return: char * (페이로드 버퍼 포인터)
		//
		// (커스텀) Header 버퍼 포인터 얻기
		//-----------------------------------------------------------------------------------------
		char				*GetHeaderBufferPtr_CustomHeader(void) { return m_cpBuffer + static_cast<int>( PACKET::HEADER_MAX_SIZE ) - m_iHeaderSize; }
	
		//-----------------------------------------------------------------------------------------
		// Param : void
		// Return: char * (Payload 버퍼 포인터)
		//
		// Payload 버퍼 포인터 얻기
		//
		// Header 이후의 Payload 버퍼 포인터 얻기
		//-----------------------------------------------------------------------------------------
		char				*GetPayloadPtr(void) { return m_cpBuffer + static_cast<int>( PACKET::HEADER_MAX_SIZE ); }

		//-----------------------------------------------------------------------------------------
		// Param : void
		// Return: (char *)버퍼 포인터
		//
		// Rear 버퍼 포인터 얻기.
		//-----------------------------------------------------------------------------------------
		char				*GetRearPtr(void) { return m_cpRear; }

		//-----------------------------------------------------------------------------------------
		// Param : void
		// Return: (char *)버퍼 포인터
		//
		// Front 버퍼 포인터 얻기.
		//-----------------------------------------------------------------------------------------
		char				*GetFrontPtr(void) { return m_cpFront; }

		//-----------------------------------------------------------------------------------------
		// Param : char * (Header 포인터)
		// Return: void
		//
		// Header 데이터 넣기
		//-----------------------------------------------------------------------------------------
		void				SetHeader(const char & cHeaderPtr);

		//-----------------------------------------------------------------------------------------
		// Param : char * (Header 포인터)
		// Return: void
		//
		// Header 데이터 넣기
		//
		// memcpy 사용 안함.
		// 포인터 이용해서 직접 넣음.
		//-----------------------------------------------------------------------------------------
		void				SetHeader_SHORT(const unsigned short shHeader);

		//-----------------------------------------------------------------------------------------
		// Param : char * (Header 포인터), int (Header 사이즈)
		// Return: void
		//
		// (커스텀) Header 데이터 넣기
		//-----------------------------------------------------------------------------------------
		void				SetHeader_CustomHeader(const char & cHeaderPtr, const int iCustomHeaderSize);

		//-----------------------------------------------------------------------------------------
		// Param :  int (이동할 사이즈)
		// Return: int (이동한 사이즈)
		//
		// Rear / Front 포지션 이동
		//-----------------------------------------------------------------------------------------
		int					MoveRear(const int iSize);
		int					MoveFront(const int iSize);

		//-----------------------------------------------------------------------------------------
		// Param :  char * (목적지 버퍼), int (쓸 사이즈)
		// Return: int (쓴 사이즈)
		//
		// 데이터 쓰기
		//-----------------------------------------------------------------------------------------
		int					Enqueue(const char &crSrc, int iSize);

		//-----------------------------------------------------------------------------------------
		// Param :  void
		// Return: void
		//
		// 패킷 암호화
		//-----------------------------------------------------------------------------------------
		void				Encode(void);
		
		//-----------------------------------------------------------------------------------------
		// Param : void
		// Return: true (성공), flase (실패)
		//
		// 패킷 복호화
		//-----------------------------------------------------------------------------------------
		bool				Decode(NET_HEADER *pHeader = nullptr);

		//-----------------------------------------------------------------------------------------
		// Param : char (패킷 내부 고정 코드), char (XOR 고정 코드), char (XOR 고정 코드)
		// Return: void
		//
		// 코드 초기화
		//-----------------------------------------------------------------------------------------
		static void			SetCode(const char cCode, const char cXORCode1, const char cXORCode2);

		//-----------------------------------------------------------------------------------------
		// Param :  char * (목적지 버퍼), int (읽을 사이즈)
		// Return: int (읽은 사이즈)
		//
		// 데이터 읽기
		//-----------------------------------------------------------------------------------------
		int					Dequeue(char *cpDest, int iSize);

		//-----------------------------------------------------------------------------------------
		// param : long (청크 사이즈)
		// return: void
		//
		// 메모리풀 생성
		//
		// 블록 개수 = 0, Alloc()으로 동적할당
		// 블록 개수 != 0, 미리 동적할당
		//-----------------------------------------------------------------------------------------
		static void			AllocMemoryPool(const long lChunkSize = 0);

		//-----------------------------------------------------------------------------------------
		// param : void
		// return: void
		//
		// 메모리풀 삭제
		//-----------------------------------------------------------------------------------------
		static void			DeleteMemoryPool(void);

		//-----------------------------------------------------------------------------------------
		// param : void
		// return: CPacket &(패킷 구조체 레퍼런스)
		//
		// 메모리 풀 할당
		//-----------------------------------------------------------------------------------------
		static CPacket *	Alloc(void);

		//-----------------------------------------------------------------------------------------
		// param : CPacket &(free할 레퍼런스)
		// return: true (반환 성공), false (m_iRefCnt 1감소)
		//
		// m_lRefCnt 감소 == 0 이면 메모리 풀 반환
		//-----------------------------------------------------------------------------------------
		static bool			Free(CPacket & Data);

		//-----------------------------------------------------------------------------------------
		// param : void
		// return: void
		//
		// m_lRefCnt 1 증가
		//-----------------------------------------------------------------------------------------
		void				AddRef(void);

		bool				EncodeStatus(void);

		// 총 만들어진 Chunk 개수	
        static long			GetChunkCount(void) { return m_pMemoryPool->GetChunkCount(); }
		// 총 사용중인 DATA 개수
		static long			GetUsingBlockCount(void) { return m_pMemoryPool->GetUsingBlockCount(); };
		// 총 사용중인 Chunk 개수
		static long			GetUsingChunkCount(void) { return m_pMemoryPool->GetUsingChunkCount(); }

	public:
		//-----------------------------------------------------------------------------------------
		// 넣기.	각 변수 타입마다 모두 만듬.
		//-----------------------------------------------------------------------------------------
		CPacket	&operator << (const BYTE &byValue);
		CPacket	&operator << (const char &chValue);

		CPacket	&operator << (const short &shValue);
		CPacket	&operator << (const WORD &wValue);

		CPacket	&operator << (const int &iValue);
		CPacket	&operator << (const DWORD &dwValue);
		CPacket	&operator << (const float &fValue);

		CPacket	&operator << (const __int64 &iValue);
		CPacket	&operator << (const double &dValue);

		CPacket	&operator << (const UINT64 &iValue);

		//-----------------------------------------------------------------------------------------
		// 빼기.	각 변수 타입마다 모두 만듬.
		//-----------------------------------------------------------------------------------------
		CPacket	&operator >> (BYTE *byValue);
		CPacket	&operator >> (char *chValue);

		CPacket	&operator >> (short *shValue);
		CPacket	&operator >> (WORD *wValue);

		CPacket	&operator >> (int *iValue);
		CPacket	&operator >> (DWORD *dwValue);
		CPacket	&operator >> (float *fValue);

		CPacket	&operator >> (__int64 *iValue);
		CPacket	&operator >> (double *dValue);

		CPacket	&operator >> (UINT64 *ui64Value);

	private:

		//-----------------------------------------------------------------------------
		// 패킷버퍼 / 버퍼 사이즈.
		//-----------------------------------------------------------------------------
		char	m_cBufferDefault[static_cast<int>( PACKET::BUFF_PACKET_1024 )];
		int		m_iBufferSize;
		int		m_iHeaderSize;

		//-----------------------------------------------------------------------------
		// 버퍼의 시작 위치, 읽을 위치, 넣을 위치.
		//-----------------------------------------------------------------------------
		char	*m_cpBuffer;
		char	*m_cpRear;
		char	*m_cpFront;

		//-----------------------------------------------------------------------------
		// 현재 버퍼에 사용중인 사이즈.
		//-----------------------------------------------------------------------------
		int		m_iUsingSize;

		public:
		//-----------------------------------------------------------------------------
		// 멤버 메모리풀
		//-----------------------------------------------------------------------------
		// tls
		static CMemoryPool_TLS<CPacket>	*m_pMemoryPool;

		public:
		//-----------------------------------------------------------------------------
		// 참조 카운터
		// 0 이면 반환
		//-----------------------------------------------------------------------------
		long		m_lRefCnt;

		//-----------------------------------------------------------------------------
		// 암호화 여부
		//-----------------------------------------------------------------------------
		bool		m_bEncodeComeplete;

		//-----------------------------------------------------------------------------
		// 암호화 & 복호화 고정 코드
		//-----------------------------------------------------------------------------
		static BYTE m_cCode;
		static BYTE m_cXORCode1;
		static BYTE m_cXORCode2;
	};
}
