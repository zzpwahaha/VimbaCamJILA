#ifndef MEMCPY_THREADED_H_
#define MEMCPY_THREADED_H_

#include <cstring>

#include <QFuture>
#include <QFutureSynchronizer>
#include <QtConcurrentRun>
/** runable for memcopy/memmove 
*/
struct copy_runable
{
    void*       m_Src;      // source data pointer
    void*       m_Dst;      // destination data pointer
    size_t      m_Size;     // size to copy
    /** run will execute the memcopy.
    */
    void run( ) const
    {
        std::memmove( m_Dst, m_Src, m_Size );
    }
};
/** thread function for a copy
*/
inline void copy_fun(copy_runable* prm)
{
    prm->run();
}
/** multi threaded mem copy
*/
template <size_t THREAD_COUNT>
inline void memcpy_threaded( void *dst,void*src, size_t len)
{
    if( len <= 0x200000) // if within L3 cache size use memcopy
    {
        std::memmove( dst, src, len);
        return;
    }
    size_t                      copy_len = len/THREAD_COUNT;
    copy_runable                prm[THREAD_COUNT];
    QFutureSynchronizer<void>   RoadBlock;
    for( size_t i = 0; i < THREAD_COUNT; ++ i)
    {
        prm[i].m_Src    = src;
        prm[i].m_Dst    = dst;
        prm[i].m_Size   = copy_len;

        RoadBlock.addFuture( QtConcurrent::run( copy_fun, &prm[i]) );

        src             = (char*)src + copy_len;
        dst             = (char*)dst + copy_len;
        len            -= copy_len;
        if( len < copy_len)
        {
            copy_len = len;
        }
    }
    RoadBlock.waitForFinished();
}

#endif