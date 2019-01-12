#ifndef __TRACYEXPLICIT_HPP__
#define __TRACYEXPLICIT_HPP__

#include <stdint.h>
#include <string.h>

#include "../common/TracySystem.hpp"
#include "../common/TracyAlign.hpp"
#include "../common/TracyAlloc.hpp"
#include "TracyProfiler.hpp"

namespace tracy
{

class ExplicitZone
{
public:
    tracy_force_inline ExplicitZone( const SourceLocationData* srcloc )
    {
        const auto thread = GetThreadHandle();
        m_thread = thread;
        m_srcloc = srcloc;
        m_depth = -1;
    }

    tracy_force_inline ExplicitZone( const SourceLocationData* srcloc, int depth )
    {
        const auto thread = GetThreadHandle();
        m_thread = thread;
        m_srcloc = srcloc;
        m_depth = depth;
    }

    tracy_force_inline void EmitBegin()
    {
        Magic magic;
        auto& token = s_token.ptr;
        auto& tail = token->get_tail_index();
        auto item = token->enqueue_begin<tracy::moodycamel::CanAlloc>(magic);
        (m_depth >= 0) ? MemWrite(&item->hdr.type, QueueType::ZoneBeginCallstack) : MemWrite(&item->hdr.type, QueueType::ZoneBegin);
        #ifdef TRACY_RDTSCP_OPT
        MemWrite(&item->zoneBegin.time, Profiler::GetTime(item->zoneBegin.cpu));
        #else
        uint32_t cpu;
        MemWrite(&item->zoneBegin.time, Profiler::GetTime(cpu));
        MemWrite(&item->zoneBegin.cpu, cpu);
        #endif
        MemWrite(&item->zoneBegin.thread, m_thread);
        MemWrite(&item->zoneBegin.srcloc, (uint64_t)m_srcloc);
        tail.store(magic + 1, std::memory_order_release);

        if(m_depth >= 0) s_profiler.SendCallstack(m_depth, m_thread);
    }

    tracy_force_inline void EmitEnd()
    {
        Magic magic;
        auto& token = s_token.ptr;
        auto& tail = token->get_tail_index();
        auto item = token->enqueue_begin<tracy::moodycamel::CanAlloc>(magic);
        MemWrite(&item->hdr.type, QueueType::ZoneEnd);
        #ifdef TRACY_RDTSCP_OPT
        MemWrite(&item->zoneEnd.time, Profiler::GetTime(item->zoneEnd.cpu));
        #else
        uint32_t cpu;
        MemWrite(&item->zoneEnd.time, Profiler::GetTime(cpu));
        MemWrite(&item->zoneEnd.cpu, cpu);
        #endif
        MemWrite(&item->zoneEnd.thread, m_thread);
        tail.store(magic + 1, std::memory_order_release);
    }

    tracy_force_inline void Text(const char* txt, size_t size)
    {
        Magic magic;
        auto& token = s_token.ptr;
        auto ptr = (char*)tracy_malloc(size + 1);
        memcpy(ptr, txt, size);
        ptr[size] = '\0';
        auto& tail = token->get_tail_index();
        auto item = token->enqueue_begin<tracy::moodycamel::CanAlloc>(magic);
        MemWrite(&item->hdr.type, QueueType::ZoneText);
        MemWrite(&item->zoneText.thread, m_thread);
        MemWrite(&item->zoneText.text, (uint64_t)ptr);
        tail.store(magic + 1, std::memory_order_release);
    }

    tracy_force_inline void Name( const char* txt, size_t size )
    {
        Magic magic;
        auto& token = s_token.ptr;
        auto ptr = (char*)tracy_malloc( size+1 );
        memcpy( ptr, txt, size );
        ptr[size] = '\0';
        auto& tail = token->get_tail_index();
        auto item = token->enqueue_begin<tracy::moodycamel::CanAlloc>( magic );
        MemWrite( &item->hdr.type, QueueType::ZoneName );
        MemWrite( &item->zoneText.thread, m_thread );
        MemWrite( &item->zoneText.text, (uint64_t)ptr );
        tail.store( magic + 1, std::memory_order_release );
    }

private:
    uint64_t m_thread;
    const SourceLocationData* m_srcloc;
    int m_depth;
};

}

#endif
