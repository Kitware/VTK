#ifndef PROJECT_VERSION_HPP
#define PROJECT_VERSION_HPP

/* Version Suite Class
 * Author: Brian Panneton
 */
#include <string>
#include <sstream>

/**
 * @brief Version Suite to assist in adding versioning to your project
 *
 * A simple way to have the library contain its own version.
 */
class ProjectVersion {
	public:
	/**
     * Create a Version class object
     *
     * @param name of the project
     */
    ProjectVersion(std::string iProjectName, int iMajor, int iMinor, int iPatch) { 
      setProjectName(iProjectName);
      setMajor(iMajor);
      setMinor(iMinor);
      setPatch(iPatch);
    }

    ProjectVersion(std::string iProjectName, 
                   std::string iMajor, std::string iMinor, std::string iPatch) { 
      setProjectName(iProjectName);
      setMajorStr(iMajor);
      setMinorStr(iMinor);
      setPatchStr(iPatch);
    }

    /**
     * Get the version string
     *
     * @return the Version in "ProjectName Major.Minor.Patch" string format
     */	
    std::string getFull() {
      return getProjectName()+std::string(" ")+
             getMajorStr()+std::string(".")+
             getMinorStr()+std::string(".")+
             getPatchStr();
    }

    /**
     * Get the shorter version string
     *
     * @return the Version in "Major.Minor" string format
     */  
    std::string getShort() {
      return getMajorStr()+std::string(".")+
             getMinorStr();
    }
	
    /**
     * Get the version objects project name
     *
     * @return the project name in string format
     */	
    std::string getProjectName() { return ProjectName; }
                
    /**
     * Get the Version Major
     *
     * @return the Version Major in string format
     */
    std::string getMajorStr() 
    { 
      if(Major != -1) return IntToStr(Major);
      return("X");
    }
		
    /**
     * Get the Version Minor
     *
     * @return the Version Minor in string format
     */
    std::string getMinorStr() 
    { 
      if(Minor != -1) return IntToStr(Minor); 
      return("X");
    }

    /**
     * Get the Version Patch
     *
     * @return the Version Patch in string format
     */
    std::string getPatchStr()
    {
      if(Patch != -1) return IntToStr(Patch);
      return("X");
    }

    /**
     * Get the Version Major
     *
     * @return the Version Major in int format
     */
    int getMajor() { return Major; }
	
    /**
     * Get the Version Minor
     *
     * @return the Version Minor in int format
     */
    int getMinor() { return Minor; }

    /**
     * Get the Version Patch
     *
     * @return the Version Patch in int format
     */
    int getPatch() { return Patch; }

private:
    std::string ProjectName;
	int Major, Minor, Patch;

    std::string IntToStr(int number) {
      std::stringstream s;
      s << number;
      return s.str();
    }
    int StrToInt(std::string string) {
      int i = 0;
      std::stringstream s(string);
      if(!(s >> i)) return -1;
      return i;
    }
    void setProjectName(std::string iProjectName)
      { ProjectName = iProjectName; }

    void setMajor(int iMajor) { Major = iMajor; }
    void setMajorStr(std::string iMajor) {
      Major = StrToInt(iMajor);
    }
    void setMinor(int iMinor) { Minor = iMinor; }
    void setMinorStr(std::string iMinor) {
      Minor = StrToInt(iMinor);
    }
    void setPatch(int iPatch) { Patch = iPatch; }
    void setPatchStr(std::string iPatch) {
      Patch = StrToInt(iPatch);
    }
};

#endif //PROJECT_VERSION_HPP
