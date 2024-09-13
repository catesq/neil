#include "faust_ui.hpp"
#include <cstring>



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


faust_widget_info* faust_ui::get_widget(const char* label) {
    for(auto& widget : widgets)
        if(widget->label == label)
            return widget;

    return nullptr;
}


void faust_ui::add_widget(const char* label, FAUSTFLOAT* zone) 
{
    widgets.push_back(new faust_widget_info{label, zone});
}
