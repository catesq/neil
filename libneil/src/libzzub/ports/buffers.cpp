#include "libzzub/ports/buffers.h"


namespace zzub {


void 
basic_buf::read(float* dest, uint num, bool delay_frame)
{
    assert(num <= block_size);

    std::copy(
        data.begin(), 
        data.begin() + num, 
        dest
    );
}


void 
basic_buf::write(float* src, uint num)
{
    assert(num <= block_size);

    if(num < block_size) {
        std::copy(
            data.begin() + (block_size - num), 
            data.end(), 
            data.begin()
        );
    }

    std::copy(
        src, 
        src + num, 
        data.begin() + (block_size - num)
    );
}



/* read from the buffer
    *
    * @param dest - destination buffer, must be zzub_buffer_size long
    * @param num - number of samples to read, up to zzub_buffer_size
    * @param delay_frame - read from 
    * @param delay_frame - whether to delay by zzub_buffer_size samples
    */
void 
basic_rb::read(float* dest, uint num, bool delay_frame)
{   
    num = num & zzub_buffer_size;
    uint offset = delay_frame ? 0 : (uint) zzub_buffer_size;
    uint ri = (wi + offset) & bufmask;
    
    if(ri + num > buf_size) {
        std::copy(
            data.begin() + ri, 
            data.begin() + buf_size, 
            dest
        );
        std::copy(
            data.begin(),
            data.begin() + ((ri + num) & bufmask), 
            dest + (buf_size - ri)
        );
    } else {
        std::copy(
            data.begin(), 
            data.begin() + ri + num, 
            dest
        );
    }
}


/**
 *  write to the buffer
 * 
 * @param src - source buffer
 * @param num - number of samples to write
 */
void 
basic_rb::write(float* src, uint num)
{
    num = num & bufmask;
    uint rem_num = buf_size - wi;

    if(rem_num < num) {
        // printf("write v1 a. buf size %d rem %d num %d wi %d\n", buf_size, rem_num, num, wi);
        std::copy(
            src, 
            src + rem_num, 
            data.begin() + wi
        );

        std::copy(
            src + rem_num, 
            src + num, 
            data.begin()
        );

        wi = num - rem_num;

    } else {
        // printf("write v2. buf size %d rem %d num %d wi %d\n", buf_size, rem_num, num, wi);

        std::copy(
            src, 
            src + num, 
            data.begin() + wi
        );

        wi = (wi + num) & bufmask;
    }
}



}