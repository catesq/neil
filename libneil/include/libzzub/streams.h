#pragma once

#include <cstring>
#include <string> 
#include <vector>
#include <iostream>
#include <cassert>

namespace zzub {




struct instream {
    virtual int read(void *buffer, int size) = 0;

    virtual long position() = 0;
    virtual void seek(long, int) = 0;

    virtual long size() = 0;

    template <typename T>
    int read(T &d) { return read(&d, sizeof(T)); }

    int read(std::string &d) {
        char c = -1;
        d = "";
        int i = 0;
        do {
            if (!read<char>(c)) break;
            if (c) d += c;
            i++;
        } while (c != 0);
        return i;
    }

    virtual ~instream() {}
};




struct outstream {
    virtual int write(void *buffer, int size) = 0;

    template <typename T>
    int write(T d) { return write(&d, sizeof(T)); }

    int write(const char *str) { return write((void *)str, (int)strlen(str) + 1); }

    virtual long position() = 0;
    virtual void seek(long, int) = 0;
    virtual ~outstream() {}
};





struct file_outstream : zzub::outstream {
    FILE* f;

    file_outstream() { f = 0; }

    bool create(const char* fn) {
        f = fopen(fn, "w+b");
        if (!f) return false;
        return true;
    }

    bool open(const char* fn) {
        f = fopen(fn, "rb");
        if (!f) return false;
        return true;
    }

    void close() {
        if (f) fclose(f);
        f = 0;
    }

    void seek(long pos, int mode=SEEK_SET) {
        fseek(f, pos, mode);
    }

    long position() {
        return ftell(f);
    }

    int write(void* p, int bytes) {
        return (int)fwrite(p, (unsigned int)bytes, 1, f);
    }

};





struct file_instream : zzub::instream {
    FILE* f;

    file_instream() { f = 0; }

    bool open(const char* fn) {
        f = fopen(fn, "rb");
        if (!f) return false;
        return true;
    }

    int read(void* v, int size) {
        int pos = position();
        int result;
        result = fread(v, 1, size, f);
        if (result != size) {
            //
        }
        return position() - pos;
    }

    void close() {
        if (f) fclose(f);
        f = 0;
    }

    void seek(long pos, int mode=SEEK_SET) {
        fseek(f, pos, mode);
    }

    long position() {
        return ftell(f);
    }

    virtual long size() {
        int prev = position();
        seek(0, SEEK_END);
        long pos = position();
        seek(prev, SEEK_SET);
        return pos;
    }


};




struct mem_outstream : zzub::outstream {
    std::vector<char> &buffer;
    int pos;

    mem_outstream(std::vector<char> &b) : buffer(b), pos(0) {}

    virtual int write(void *buffer, int size) {
        char *charbuf = (char*)buffer;
        int ret = size;
        if (pos + size > (int)this->buffer.size()) this->buffer.resize(pos+size);
        while (size--) {
            //this->buffer.push_back(*charbuf++);
            this->buffer[pos++] = *charbuf++;
        }
        return ret;
    }

    virtual long position() {
        return pos;
    }

    virtual void seek(long offset, int origin) {
        if (origin == SEEK_SET) {
            pos = offset;
        } else if (origin == SEEK_CUR) {
            pos += offset;
        } else if (origin == SEEK_END) {
            pos = (int)this->buffer.size() - offset;
        } else {
            assert(0);
        }
    }
};

struct mem_instream : zzub::instream {
    long pos;
    std::vector<char> &buffer;

    mem_instream(std::vector<char> &b) : buffer(b) { pos = 0; }

    virtual int read(void *buffer, int size) {
        if (pos + size > (int)this->buffer.size()) {
            std::cerr << "Tried to read beyond end of memory stream" << std::endl;
            size = (int)this->buffer.size() - pos;
        }
        memcpy(buffer, &this->buffer[pos], size);
        pos += size;
        return size;
    }

    virtual long position() {
        return pos;
    }

    virtual void seek(long offset, int origin) {
        if (origin == SEEK_SET) {
            pos = offset;
        } else if (origin == SEEK_CUR) {
            pos += offset;
        } else if (origin == SEEK_END) {
            pos = (int)this->buffer.size() - offset;
        } else {
            assert(0);
        }
    }

    virtual long size() { return (long)buffer.size(); }
};

}