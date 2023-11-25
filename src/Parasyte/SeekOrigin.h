#pragma once

namespace ps
{
    // Seek Direction
    enum class SeekOrigin
    {
        // Specifies the beginning of a stream.
        Begin = 0,
        // Specifies the current position within a stream.
        Current = 1,
        // Specifies the end of a stream.
        End = 2,
    };
}

