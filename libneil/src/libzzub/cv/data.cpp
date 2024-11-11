#include "libzzub/cv/data.h"


namespace zzub {


/***************************
 * 
 ***************************/

cv_data_type 
get_data_type(zzub::metaplugin& meta, const cv_node& node, zzub::port_flow flow) 
{
    auto port_type = static_cast<zzub::port_type>(node.port_type);

    auto port = meta.plugin->get_port(port_type, flow, node.value);

    switch(port_type) {
        case port_type::audio:
            return cv_data_type::zzub_audio;

        case port_type::track:
        case port_type::param:
            if (port)
                return cv_data_type::cv_param;
            else
                return cv_data_type::zzub_param;

        case port_type::cv:
            if(!port) {
                printf("cv port problem in data source: port number %d in plugin %s\n", node.value, meta.info->name.c_str());
                assert(false);
            }

            switch(port->get_type()) {
                case port_type::param:
                case port_type::track:
                    return cv_data_type::cv_param;

                default:
                    return cv_data_type::cv_stream;
            }
    }

    assert(false);
}


/***************************
 * 
 ***************************/

cv_data*
cv_data::create(cv_data_type data_type) {
    switch(data_type) {
        case cv_data_type::zzub_audio:
            return new cv_data_stream(data_type);

        case cv_data_type::zzub_param:
            return new cv_data_param(data_type);

        case cv_data_type::cv_param:
            return new cv_data_param(data_type);

        case cv_data_type::cv_stream:
            return new cv_data_stream(data_type);
    }

    assert(false);
}


/**************************
 * 
 * cv_data_param
 * 
 **************************/

void 
cv_data_param::reset(
    int numsamples
)
{
    value = 0;
};

// complete the virtual methods of cv_data_param and cv_data_stream
void 
cv_data_param::add(
    float value
)
{
    this->value += value;
}


void 
cv_data_param::add(
    float* value,
    int numsamples
)
{
    float sum = 0;

    for (int i = 0; i < numsamples; i++) {
        sum += std::abs(*value++);
    }

    this->value += sum / numsamples;
}



float 
cv_data_param::get( )
{
    return value;
};


/**
 * the param and stream have a matching transporter 
 * so only the correct get method is called 
 * if this one is called then something is wrong 
 */
float* 
cv_data_param::get(int numsamples)
{
    assert(false);
};

/***************************
 * 
 * cv_data_stream
 * 
 ***************************/


/**
 * 
 */
void 
cv_data_stream::reset(
    int count 
)
{
    if(count <= 0)
        return;

    // copy <count> samples from the end to the start of data
    // memcpy(data.data(), &data[zzub_buffer_size - count], count * sizeof(float));
    std::copy(data.begin() + zzub_buffer_size - count, data.end(), data.begin());
    //set the last <count> elements to 0
    std::fill(data.end() - count, data.end(), 0);
    // memset(&data[zzub_buffer_size - count], 0, count * sizeof(float));
}


void 
cv_data_stream::add(
    float value
)
{
    for (auto& it : data) {
        it += value;
    }
}


void 
cv_data_stream::add(
    float* value,
    int numsamples
)
{
    float* dest = &data[zzub_buffer_size - numsamples];
    for (int i = 0; i < numsamples; i++) {
        *dest++ += *value++;
    }
}


/**
 * the param and stream have a matching transporter 
 * so only the correct get method is called 
 * if this one is called then something is wrong 
 */
float 
cv_data_stream::get()
{
    assert(false);
};


float* 
cv_data_stream::get(int numsamples)
{
    return &data.front();
};



}