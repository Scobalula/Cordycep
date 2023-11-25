#pragma once
#include "SeekOrigin.h"
namespace ps
{
    class BinaryReader
    {
    private:
        std::ifstream Stream;
        std::streampos Length;
    public:
        // Opens a new stream and disposes of the previous one (if open)
        void Open(std::string filePath)
        {
            Stream.close();
            Stream = std::ifstream(filePath, std::ios::binary);

            if (!Stream)
            {
                Stream.close();
                throw std::exception("PLX::BinaryReader: Failed to create Stream for BinaryReader");
            }
            else
            {
                Stream.exceptions(std::ifstream::failbit | std::ifstream::badbit);
                Stream.seekg(0, std::ios::end);
                Length = Stream.tellg();
                Stream.seekg(0, std::ios::beg);
            }
        }
        // Inits a new Binary Reader
        BinaryReader() {}
        // Inits a new Binary Reader and opens the file
        BinaryReader(std::string filePath)
        {
            Open(filePath);
        }
        // Reads a struct/type from the stream of the given type
        template <class T>
        T Read()
        {
            T result{};
            ReadBytes((char*)&result, sizeof(result));
            return result;
        }
        // Reads a struct/type from the stream of the given type
        template <class T>
        T Read(std::streamoff offset)
        {
            auto currentPos = GetPosition();
            Seek(offset, SeekOrigin::Begin);

            T result{};
            ReadBytes((char*)&result, sizeof(result));

            Seek(currentPos, SeekOrigin::Begin);

            return result;
        }
        // Reads an array of the given struct/type from the stream of the given type
        template <class T>
        std::vector<T> ReadArray(size_t size)
        {
            std::vector<T> results(size);
            ReadBytes((char*)&results.front(), sizeof(T) * size);
            return results;
        }
        // Reads an array of the given struct/type from the stream of the given type
        template <class T>
        std::vector<T> ReadArray(size_t size, std::streamoff offset)
        {
            auto currentPos = GetPosition();
            Seek(offset, SeekOrigin::Begin);

            std::vector<T> results(size);
            ReadBytes((char*)&results.front(), sizeof(T) * size);

            Seek(currentPos, SeekOrigin::Begin);

            return results;
        }
        // Reads a string terminated by a null byte
        std::string ReadNullTerminatedString()
        {
            std::stringstream result;
            auto character = Read<char>();

            while (character != 0)
            {
                result << character;
                character = Read<char>();
            }

            return result.str();
        }
        // Reads a string terminated by a null byte
        std::string ReadNullTerminatedString(std::streamoff offset)
        {
            auto currentPos = GetPosition();
            Seek(offset, SeekOrigin::Begin);

            std::stringstream result;
            auto character = Read<char>();

            while (character != 0)
            {
                result << character;
                character = Read<char>();
            }

            Seek(currentPos, SeekOrigin::Begin);

            return result.str();
        }
        // Reads bytes from the stream
        std::unique_ptr<char[]> ReadBytes(size_t count, size_t& bytesRead)
        {
            auto result = std::make_unique<char[]>(count);
            bytesRead = ReadBytes(result.get(), count);
            return result;
        }
        // Reads bytes from the stream
        std::unique_ptr<char[]> ReadBytes(size_t count, size_t& bytesRead, std::streamoff offset)
        {
            auto currentPos = GetPosition();
            Seek(offset, SeekOrigin::Begin);

            auto result = std::make_unique<char[]>(count);
            bytesRead = ReadBytes(result.get(), count);

            Seek(currentPos, SeekOrigin::Begin);

            return result;
        }
        // Reads bytes from the stream
        virtual size_t ReadBytes(char* data, size_t count)
        {
            size_t result = 0;

            if (Stream)
            {
                Stream.read(data, count);
                result = (size_t)Stream.gcount();
            }

            return result;
        }
        // Reads bytes from the stream
        size_t ReadBytes(char* data, size_t count, std::streamoff offset)
        {
            auto currentPos = GetPosition();
            Seek(offset, SeekOrigin::Begin);

            auto result = ReadBytes(data, count);

            Seek(currentPos, SeekOrigin::Begin);

            return result;
        }
        // Seeks to the position in the stream
        virtual void Seek(std::streamoff offset, SeekOrigin origin)
        {
            Stream.seekg(offset,
                origin == SeekOrigin::Begin ? std::ios::beg :
                origin == SeekOrigin::End ? std::ios::end : std::ios::cur);
        }
        // Seeks to the position in the stream from the start
        virtual void Seek(std::streamoff offset)
        {
            Seek(offset, SeekOrigin::Begin);
        }
        // Gets the current stream position
        virtual std::streamoff GetPosition()
        {
            return Stream.tellg();
        }
        // Gets the current stream position
        virtual std::streampos GetLength()
        {
            return Length;
        }
    };
}