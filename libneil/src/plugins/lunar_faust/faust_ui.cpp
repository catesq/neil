#include "faust_ui.hpp"




void faust_ui::addButton(const char* label, FAUSTFLOAT* zone)
{
    add_widget(label, zone);
}


void faust_ui::addCheckButton(const char* label, FAUSTFLOAT* zone)
{
    add_widget(label, zone);
}


void faust_ui::addVerticalSlider(const char* label, FAUSTFLOAT* zone, FAUSTFLOAT init, FAUSTFLOAT min, FAUSTFLOAT max, FAUSTFLOAT step)
{
    add_widget(label, zone);
}


void faust_ui::addHorizontalSlider(const char* label, FAUSTFLOAT* zone, FAUSTFLOAT init, FAUSTFLOAT min, FAUSTFLOAT max, FAUSTFLOAT step)
{
    add_widget(label, zone);
}


void faust_ui::addNumEntry(const char* label, FAUSTFLOAT* zone, FAUSTFLOAT init, FAUSTFLOAT min, FAUSTFLOAT max, FAUSTFLOAT step)
{
    add_widget(label, zone);
}


void faust_ui::addHorizontalBargraph(const char* label, FAUSTFLOAT* zone, FAUSTFLOAT min, FAUSTFLOAT max)
{
    add_widget(label, zone);
}


void faust_ui::addVerticalBargraph(const char* label, FAUSTFLOAT* zone, FAUSTFLOAT min, FAUSTFLOAT max)
{
    add_widget(label, zone);
}


int faust_ui::get_widget_index(const char* label)
{
    for(int i = 0; i < labels.size(); i++)
        if(labels[i] == label)
            return i;

    return -1;
}


FAUSTFLOAT faust_ui::get_widget_value(int index)
{
    if(index < values.size())
        return *values[index];

    return 0;
}


void faust_ui::set_widget_value(int index, FAUSTFLOAT new_value)
{
    if(index < values.size())
        *values[index] = new_value;
}


void faust_ui::add_widget(const char*label, FAUSTFLOAT* zone) {
    labels.push_back(label);
    values.push_back(zone);
}
