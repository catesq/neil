
from configparser import ConfigParser

# the package info
class PackageInfo:
    def __init__(self):
        self.module = None
        self.name = None
        self.description = None
        self.icon = None
        self.authors = None
        self.website = None

    def is_valid(self):
        return self.module and self.name
    
    def __str__(self):
        return self.name + " (" + self.module + ")"
    


# get package info from a .neil-component 
class PackageParser(ConfigParser):
    section_name = 'Neil COM'

    options = [
        'Module',
        'Name',
        'Description',
        'Icon',
        'Authors',
        'Copyright',
        'Website',
    ]
    
    def parse_package(self, filename):
        self.remove_section(self.section_name)
        self.read([filename])

        package = PackageInfo()

        if not self.has_section(self.section_name):
            print("missing section " + self.section_name + " in " + filename)
            return None
        
        for option in self.options:
            if self.has_option(self.section_name, option):
                setattr(package, option.lower(), self.get(self.section_name, option))
            else:
                print("missing component info " + option + " in " + filename)
                
        return package