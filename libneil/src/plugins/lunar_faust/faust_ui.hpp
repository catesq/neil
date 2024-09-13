#include "faust/gui/UI.h"
#include <vector>
#include <string>

struct faust_widget_info
{
    std::string label;
    FAUSTFLOAT *value;
};


struct faust_ui : UI
{
    virtual void openTabBox(const char* label) {}
    virtual void openHorizontalBox(const char* label) {}
    virtual void openVerticalBox(const char* label) {}
    virtual void closeBox() {}
    
    // -- active widgets
    
    virtual void addButton(const char* label, FAUSTFLOAT* zone) override;
    virtual void addCheckButton(const char* label, FAUSTFLOAT* zone) override;
    virtual void addVerticalSlider(const char* label, FAUSTFLOAT* zone, FAUSTFLOAT init, FAUSTFLOAT min, FAUSTFLOAT max, FAUSTFLOAT step) override;
    virtual void addHorizontalSlider(const char* label, FAUSTFLOAT* zone, FAUSTFLOAT init, FAUSTFLOAT min, FAUSTFLOAT max, FAUSTFLOAT step) override;
    virtual void addNumEntry(const char* label, FAUSTFLOAT* zone, FAUSTFLOAT init, FAUSTFLOAT min, FAUSTFLOAT max, FAUSTFLOAT step) override;
    
    // -- passive widgets
    
    virtual void addHorizontalBargraph(const char* label, FAUSTFLOAT* zone, FAUSTFLOAT min, FAUSTFLOAT max) override;
    virtual void addVerticalBargraph(const char* label, FAUSTFLOAT* zone, FAUSTFLOAT min, FAUSTFLOAT max) override;
    
    // -- soundfiles
    
    virtual void addSoundfile(const char* label, const char* filename, Soundfile** sf_zone) {}


    faust_widget_info* get_widget(const char* label);

private:
    void add_widget(const char*, FAUSTFLOAT*);

    std::vector<faust_widget_info*> widgets;
};