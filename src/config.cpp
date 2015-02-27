/*************************************************************************
 *
 * Copyright (C) 1997-2014 by Dimitri van Heesch.
 * Copyright (C) 2014-2015 Barbara Geller & Ansel Sermersheim
 * All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation under the terms of the GNU General Public License version 2
 * is hereby granted. No representations are made about the suitability of
 * this software for any purpose. It is provided "as is" without express or
 * implied warranty. See the GNU General Public License for more details.
 *
 * Documents produced by DoxyPress are derivative works derived from the
 * input used in their production; they are not affected by this license.
 *
*************************************************************************/

#include <QDir>

#include <config.h>
//   #include <doxy_build_info.h>
#include <doxy_setup.h>
#include <doxy_globals.h>
#include <language.h>
#include <message.h>
#include <pre.h>
//   #include <portable.h>
#include <util.h>

bool Config::getBool(const QString name)
{
   bool retval   = false;  
   auto hashIter = m_cfgBool.find(name);

    if (hashIter != m_cfgBool.end()) {
      retval = hashIter.value().value;

   } else { 
      fprintf(stderr, "Warning: %s was not retrieved from the project bool table\n", qPrintable(name) );
   
   }   
  
   return retval;
}

QString Config::getEnum(const QString name)
{
   QString retval;   
   auto hashIter = m_cfgEnum.find(name);

    if (hashIter != m_cfgEnum.end()) {
      retval = hashIter.value().value;

   } else { 
      fprintf(stderr, "Warning: %s was not retrieved from the project enum table\n", qPrintable(name) );
   
   }   

   return retval;
}

int Config::getInt(const QString name)
{
   int retval    = 0;   
   auto hashIter = m_cfgInt.find(name);

    if (hashIter != m_cfgInt.end()) {
      retval = hashIter.value().value;

   } else { 
      fprintf(stderr, "Warning: %s was not retrieved from the project integer table\n", qPrintable(name) );
   
   }   

   return retval;
}

QStringList Config::getList(const QString name)
{
   QStringList retval;
   auto hashIter = m_cfgList.find(name);

    if (hashIter != m_cfgList.end()) {
      retval = hashIter.value().value;

   } else { 
      fprintf(stderr, "Warning: %s was not retrieved from the project list table\n", qPrintable(name) );
   
   }   
  
   return retval;
}

QString Config::getString(const QString name)
{
   QString retval;
   auto hashIter = m_cfgString.find(name);

    if (hashIter != m_cfgString.end()) {
      retval = hashIter.value().value;

   } else { 
      fprintf(stderr, "Warning: %s was not retrieved from the project string table\n", qPrintable(name) );
   
   }   
  
   return retval;
}

// update project data
void Config::setList(const QString name, QStringList data)
{
   auto hashIter = m_cfgList.find(name);     
   hashIter.value().value = data;
}

void Config::verify()
{
   printf("**  Adjust Project Configuration\n");   

   //  Config::instance()->substituteEnvironmentVars();        // not used at this time (broom) 

   preVerify();
   initWarningFormat();

   // **
   auto iterString = m_cfgString.find("output-dir");
   QString outputDirectory = iterString.value().value;

   if (outputDirectory.isEmpty()) {
       outputDirectory = QDir::currentPath().toUtf8();
   
   } else {
      QDir dir(outputDirectory);

      if (! dir.exists()) {
         dir.setPath(QDir::currentPath());

         if (! dir.mkdir(outputDirectory)) {
            err("Output directory `%s' does not exist and can not be created\n", qPrintable(outputDirectory));            
            exit(1);

         } else {
            msg("Output directory `%s' was automatically created\n", qPrintable(outputDirectory));

         }

         dir.cd(outputDirectory);
      }

      outputDirectory = dir.absolutePath().toUtf8();
   }

   iterString.value().value = outputDirectory;

   // ** 
   iterString = m_cfgString.find("layout-file");
   QString layoutFileName = iterString.value().value;

   if (layoutFileName.isEmpty()) {
      layoutFileName    = "doxy_layout.xml";      
   }

   iterString.value().value = layoutFileName;

   // **
   iterString = m_cfgString.find("latex-bib-style");
   QString style = iterString.value().value;

   if (style.isEmpty()) {
      style = "plain";
   }

   iterString.value().value = style;

   // **
   iterString = m_cfgString.find("output-language");
   QString outputLanguage = iterString.value().value;

   if (outputLanguage.isEmpty()) {
      outputLanguage = "English";
   }

   if (! setTranslator(outputLanguage)) {
      warn_uncond("Output language %s not supported, using English\n", qPrintable(outputLanguage));
      outputLanguage = "English";
   }

   iterString.value().value = outputLanguage;

   
   // **
   iterString = m_cfgString.find("html-file-extension");
   QString htmlFileExtension = iterString.value().value.trimmed();

   if (htmlFileExtension.isEmpty()) {
      htmlFileExtension = ".html";
   }

   iterString.value().value   = htmlFileExtension;
   Doxygen::htmlFileExtension = htmlFileExtension;


   // ** Save data to structers and variables   

   // **   
   Doxygen::parseSourcesNeeded = Config::getBool("dot-call") ||  Config::getBool("dot-called-by") ||
                                 Config::getBool("ref-relation") || Config::getBool("ref-by-relation");
   
   Doxygen::markdownSupport    = Config::getBool("markdown");


   // ** add predefined macro name to a dictionary
   const QStringList expandAsDefinedList = Config::getList("expand-as-defined");

   for (auto s : expandAsDefinedList) {
      if (! Doxygen::expandAsDefinedDict.contains(s)) {
         Doxygen::expandAsDefinedDict.insert(s);
      }      
   }

   // ** Add custom extension mappings 
   const QStringList extMaps = Config::getList("extension-mapping");
  
   for (auto mapStr : extMaps) { 
      int i = mapStr.indexOf('=');

      if (i != -1) {
         QString extension = mapStr.left(i).trimmed().toLower();
         QString language  = mapStr.mid(i + 1).trimmed().toLower();

         if (! updateLanguageMapping(extension, language)) {
            err("Unable to map file extension '%s' to '%s', verify the 'EXTENSION MAPPING' tag\n", qPrintable(extension), qPrintable(language));

         } else {
            msg("Adding custom extension mapping: .%s will be treated as language %s\n", qPrintable(extension), qPrintable(language));
         }
      }
   } 

   // **
   const QStringList includePath = Config::getList("include-path");
 
   for (auto s : includePath) { 
      QFileInfo fi(s);
      addSearchDir(fi.absoluteFilePath());      
   }    

   // read aliases and store them in a dictionary
   readAliases();
}

void Config:: preVerify()
{
   // **
   auto iterString = m_cfgString.find("warn-format");
   QString warnFormat = iterString.value().value.trimmed();

   if (warnFormat.isEmpty()) {
      warnFormat = "$file:$line $text";

   } else {
      if (warnFormat.indexOf("$file") == -1) {
         err("Warning: warning format does not contain $file\n");
      }

      if (warnFormat.indexOf("$line") == -1) {
         err("Warning: warning format does not contain $line\n");
      }

      if (warnFormat.indexOf("$text") == -1) {
         err("Warning: warning format foes not contain $text\n");
      }
   }

   iterString.value().value = warnFormat;


   // **
   iterString = m_cfgString.find("man-extension");
   QString manExtension = iterString.value().value;

   // set default man page extension if non is given by the user
   if (manExtension.isEmpty()) {
      manExtension = "3";

   } else {

      if (manExtension.at(0) == '.') {
         if (manExtension.length() == 1) {
            manExtension = "3";
   
         } else { 
            // strip .
            manExtension = manExtension.mid(1);
         }
      }
   
      if (manExtension.at(0) < '0' || manExtension.at(0) > '9') {
         manExtension.prepend("3");
      }
   }

   iterString.value().value = manExtension;


   // **
   auto iterEnum = m_cfgEnum.find("latex-paper-type");
   QString paperType = iterEnum.value().value;

   paperType = paperType.toLower().trimmed();

   if (paperType.isEmpty()) {
      paperType = "a4";
   }

   if (paperType != "a4" && paperType != "a4wide" && paperType != "letter" && paperType != "legal" && paperType != "executive") {
      err("Warning: : Unknown paper type %s specified, using a4\n", qPrintable(paperType));
      paperType = "a4";
   }

   iterEnum.value().value = paperType;


/*
   


   // ** expand the relative stripFromPath values
   auto iterList = m_cfgList.find("");
   QStringList stripFromPath = Config_getList("STRIP_FROM_PATH");

   if ( stripFromPath.isEmpty() ) { 
      // by default use the current path
      stripFromPath.append(QDir::currentPath().toUtf8() + "/");

   } else {
      cleanUpPaths(stripFromPath);

   }

   iterList.value().value = ?;


   // ** expand the relative stripFromPath values
   iterList = m_cfgList.find("");
   QStringList stripFromIncPath = Config_getList("STRIP_FROM_INC_PATH");

   cleanUpPaths(stripFromIncPath);

   iterList.value().value = ?;


   // ** see if HTML header is valid
   iterString = m_cfgString.find("");
   QByteArray &headerFile = Config_getString("HTML_HEADER");

   if (! headerFile.isEmpty()) {

      QFileInfo fi(headerFile);

      if (! fi.exists()) {
         err("Error: tag HTML_HEADER: header file `%s' does not exist\n", headerFile.data());
         exit(1);
      }
   }

   iterString.value().value = ?;


   // ** Test to see if HTML footer is valid
   iterString = m_cfgString.find("");
   QByteArray &footerFile = Config_getString("HTML_FOOTER");

   if (! footerFile.isEmpty()) {
      QFileInfo fi(footerFile);
      if (!fi.exists()) {
         err("Error: tag HTML_FOOTER: footer file `%s' does not exist\n", footerFile.data());
         exit(1);
      }
   }


   // ** Test to see if MathJax code file is valid
   iterString = m_cfgString.find("");

   if (Config_getBool("USE_MATHJAX")) {
      QByteArray &MathJaxCodefile = Config_getString("MATHJAX_CODEFILE");

      if (!MathJaxCodefile.isEmpty()) {
         QFileInfo fi(MathJaxCodefile);

         if (!fi.exists()) {
            err("Error: tag MATHJAX_CODEFILE file `%s' does not exist\n", MathJaxCodefile.data());
            exit(1);
         }
      }
   }

   // Test to see if LaTeX header is valid
   iterString = m_cfgString.find("");
   QByteArray &latexHeaderFile = Config_getString("LATEX_HEADER");

   if (!latexHeaderFile.isEmpty()) {
      QFileInfo fi(latexHeaderFile);
      if (!fi.exists()) {
         err("Error: tag LATEX_HEADER: header file `%s' does not exist\n", latexHeaderFile.data());
         exit(1);
      }
   }
   // check include path
   iterList = m_cfgList.find("");
   QStringList includePath = Config_getList("INCLUDE_PATH");

   for (auto s : includePath) {   
      QFileInfo fi(s);

      if (! fi.exists()) {
         err("Warning: tag INCLUDE_PATH: include path `%s' does not exist\n", qPrintable(s));
      }  
   }

   // check aliases
   QStringList &aliasList = Config_getList("ALIASES");
 
   for (auto s: aliasList) {
      QRegExp re1("[a-z_A-Z][a-z_A-Z0-9]*[ \t]*=");         // alias without argument
      QRegExp re2("[a-z_A-Z][a-z_A-Z0-9]*\\{[0-9]*\\}[ \t]*="); // alias with argument

      QByteArray alias = s.toUtf8();
      alias = alias.trimmed();

      if (! (re1.indexIn(alias) == 0 || re2.indexIn(alias) == 0)) {
         err("Error: Alias format: `%s' \nForma is invalid, use \"name=value\" or \"name{n}=value\", where n is the number of arguments\n\n",
                    alias.data());
      }
      
   }

   // **
   if (Config::getBool("generate-treeview") && Config::getBool("generate-chm")) {
      err("Error: When enabling 'GENERATE CHM' the 'GENERATE TREEVIEW' tag must be disabled\n");
      Config::getBool("generate-treeview") = false;
   }

   if (Config::getBool("html-search") && Config::getBool("generate-chm")) {
      err("Error: When enabling 'GENERATE CHM' the 'HTML SEARCH' must be disabled\n");
      Config::getBool("html-search") = false;
   }

   // check if SEPARATE_MEMBER_PAGES and INLINE_GROUPED_CLASSES are both enabled
   if (Config_getBool("SEPARATE_MEMBER_PAGES") && Config_getBool("INLINE_GROUPED_CLASSES")) {
      err("Error: When enabling INLINE_GROUPED_CLASSES the SEPARATE_MEMBER_PAGES option should be disabled. I'll do it for you.\n");
      Config_getBool("SEPARATE_MEMBER_PAGES") = FALSE;
   }

   // check dot image format
   iterEnum = m_cfgEnum.find("");
   QByteArray &dotImageFormat = Config_getEnum("DOT_IMAGE_FORMAT");

   dotImageFormat = dotImageFormat.trimmed();
   if (dotImageFormat.isEmpty()) {
      dotImageFormat = "png";
   }

   //else if (dotImageFormat!="gif" && dotImageFormat!="png" && dotImageFormat!="jpg")
   //{
   //  err("Invalid value for DOT_IMAGE_FORMAT: `%s'. Using the default.\n",dotImageFormat.data());
   //  dotImageFormat = "png";
   //}

   QByteArray &dotFontName = Config_getString("DOT_FONTNAME");
   if (dotFontName == "FreeSans" || dotFontName == "FreeSans.ttf") {
      err("Warning: doxygen no longer ships with the FreeSans font.\n"
                 "You may want to clear or change DOT_FONTNAME.\n"
                 "Otherwise you run the risk that the wrong font is being used for dot generated graphs.\n");
   }


   // check dot path
   iterString = m_cfgString.find("");
   QByteArray &dotPath = Config_getString("DOT_PATH");

   if (! dotPath.isEmpty()) {

      QFileInfo fi(dotPath);
      if (fi.exists() && fi.isFile()) { // user specified path + exec
         dotPath = fi.absolutePath().toUtf8() + "/";

      } else {
         QFileInfo dp(dotPath + "/dot" + portable_commandExtension());
         if (!dp.exists() || !dp.isFile()) {
            err("Warning: the dot tool could not be found at %s\n", dotPath.data());
            dotPath = "";
         } else {
            dotPath = dp.absolutePath().toUtf8() + "/";
         }
      }

#if defined(_WIN32) // convert slashes
      uint i = 0, l = dotPath.length();

      for (i = 0; i < l; i++) if (dotPath.at(i) == '/') {
            dotPath[i] = '\\';
         }
#endif

   } else { 
      // make sure the string is empty but not null!
      dotPath = "";
   }


   // check mscgen path
   iterString = m_cfgString.find("");
   QString mscgenPath = Config_getString("MSCGEN_PATH");

   if (!mscgenPath.isEmpty()) {
      QFileInfo dp(mscgenPath + "/mscgen" + portable_commandExtension());

      if (!dp.exists() || !dp.isFile()) {
         err("Warning: the mscgen tool could not be found at %s\n", mscgenPath.data());
         mscgenPath = "";

      } else {
         mscgenPath = dp.absolutePath().toUtf8() + "/";

#if defined(_WIN32) // convert slashes
         uint i = 0, l = mscgenPath.length();

         for (i = 0; i < l; i++) if (mscgenPath.at(i) == '/') {
               mscgenPath[i] = '\\';
            }
#endif
      }
   } else { // make sure the string is empty but not null!
      mscgenPath = "";
   }


   // check plantuml path
   iterString = m_cfgString.find("");
   QString plantumlJarPath = Config_getString("PLANTUML_JAR_PATH");

   if (!plantumlJarPath.isEmpty()) {
      QFileInfo pu(plantumlJarPath);

      if (pu.exists() && pu.isDir()) { // PLANTUML_JAR_PATH is directory
         QFileInfo jar(plantumlJarPath + portable_pathSeparator() + "plantuml.jar");

         if (jar.exists() && jar.isFile()) {
            plantumlJarPath = jar.absolutePath().toUtf8() + portable_pathSeparator();
         } else {
            err("Jar file plantuml.jar not found at location "
                       "specified via PLANTUML_JAR_PATH: '%s'\n", plantumlJarPath.data());
            plantumlJarPath = "";
         }
      } else if (pu.exists() && pu.isFile() && plantumlJarPath.right(4) == ".jar") { // PLANTUML_JAR_PATH is file
         plantumlJarPath = pu.absolutePath().toUtf8() + portable_pathSeparator();
      } else {
         err("path specified via PLANTUML_JAR_PATH does not exist or not a directory: %s\n",
                    plantumlJarPath.data());
         plantumlJarPath = "";
      }
   }


   // check dia path
   iterString = m_cfgString.find("");
   QString diaPath = Config_getString("DIA_PATH");

   if (!diaPath.isEmpty()) {
      QFileInfo dp(diaPath + "/dia" + portable_commandExtension());

      if (!dp.exists() || !dp.isFile()) {
         err("Warning: dia could not be found at %s\n", diaPath.data());
         diaPath = "";

      } else {
         diaPath = dp.absolutePath().toUtf8() + "/";

#if defined(_WIN32) // convert slashes
         uint i = 0, l = diaPath.length();

         for (i = 0; i < l; i++) {
            if (diaPath.at(i) == '/') {
               diaPath[i] = '\\';
            }
         }
#endif
      }

   } else { // make sure the string is empty but not null!
      diaPath = "";
   }

   // check input
   QStringList inputSources = Config::getList("input-source")

   if (inputSources.count() == 0) {
      // use current dir as the default
      inputSources.append(QDir::currentPath().toUtf8());

   } else {    
      for (auto s : inputSources) {
         QFileInfo fi(s);

         if (!fi.exists()) {
            err("Warning: tag INPUT: input source `%s' does not exist\n", qPrintable(s));
         }         
      }
   }

   // add default pattern if needed
   QStringList &filePatternList = Config_getList("FILE_PATTERNS");
   if (filePatternList.isEmpty()) {
      filePatternList.append("*.c");
      filePatternList.append("*.cc");
      filePatternList.append("*.cxx");
      filePatternList.append("*.cpp");
      filePatternList.append("*.c++");
      //filePatternList.append("*.d");
      filePatternList.append("*.java");
      filePatternList.append("*.ii");
      filePatternList.append("*.ixx");
      filePatternList.append("*.ipp");
      filePatternList.append("*.i++");
      filePatternList.append("*.inl");
      filePatternList.append("*.h");
      filePatternList.append("*.hh");
      filePatternList.append("*.hxx");
      filePatternList.append("*.hpp");
      filePatternList.append("*.h++");
      filePatternList.append("*.idl");
      filePatternList.append("*.odl");
      filePatternList.append("*.cs");
      filePatternList.append("*.php");
      filePatternList.append("*.php3");
      filePatternList.append("*.inc");
      filePatternList.append("*.m");
      filePatternList.append("*.mm");
      filePatternList.append("*.dox");
      filePatternList.append("*.py");
      filePatternList.append("*.f90");
      filePatternList.append("*.f");
      filePatternList.append("*.for");
      filePatternList.append("*.tcl");
      filePatternList.append("*.md");
      filePatternList.append("*.markdown");

      if (portable_fileSystemIsCaseSensitive() == Qt::CaseSensitive) {
         // unix => case sensitive match => also include useful uppercase versions

         filePatternList.append("*.C");
         filePatternList.append("*.CC");
         filePatternList.append("*.C++");
         filePatternList.append("*.II");
         filePatternList.append("*.I++");
         filePatternList.append("*.H");
         filePatternList.append("*.HH");
         filePatternList.append("*.H++");
         filePatternList.append("*.CS");
         filePatternList.append("*.PHP");
         filePatternList.append("*.PHP3");
         filePatternList.append("*.M");
         filePatternList.append("*.MM");
         filePatternList.append("*.PY");
         filePatternList.append("*.F90");
         filePatternList.append("*.F");
         filePatternList.append("*.TCL");
         filePatternList.append("*.MD");
         filePatternList.append("*.MARKDOWN");
      }
   }

   // add default pattern if needed
   QStringList &examplePatternList = Config_getList("EXAMPLE_PATTERNS");
   if (examplePatternList.isEmpty()) {
      examplePatternList.append("*");
   }

   // if no output format is enabled, warn the user
   if (!Config_getBool("GENERATE_HTML")    &&
         !Config_getBool("GENERATE_LATEX")   &&
         !Config_getBool("GENERATE_MAN")     &&
         !Config_getBool("GENERATE_RTF")     &&
         !Config_getBool("GENERATE_XML")     &&
         !Config_getBool("GENERATE_PERLMOD") &&
         !Config_getBool("GENERATE_RTF")     &&
         !Config_getBool("GENERATE_AUTOGEN_DEF") &&
         Config_getString("GENERATE_TAGFILE").isEmpty()
      ) {
      err("Warning: No output formats selected! Set at least one of the main GENERATE_* options to YES.\n");
   }

   // check HTMLHELP creation requirements
   if (!Config_getBool("GENERATE_HTML") &&
         Config_getBool("GENERATE_HTMLHELP")) {
      err("Warning: GENERATE_HTMLHELP=YES requires GENERATE_HTML=YES.\n");
   }

   // check QHP creation requirements
   if (Config_getBool("GENERATE_QHP")) {
      if (Config_getString("QHP_NAMESPACE").isEmpty()) {
         err("Error: GENERATE_QHP=YES requires QHP_NAMESPACE to be set. Using 'org.doxypress.doc' as default.\n");
         Config_getString("QHP_NAMESPACE") = "org.doxypress.doc";
      }

      if (Config_getString("QHP_VIRTUAL_FOLDER").isEmpty()) {
         err("Error: GENERATE_QHP=YES requires QHP_VIRTUAL_FOLDER to be set. Using 'doc' as default\n");
         Config_getString("QHP_VIRTUAL_FOLDER") = "doc";
      }
   }

   if (Config_getBool("OPTIMIZE_OUTPUT_JAVA") && Config_getBool("INLINE_INFO")) {
      // don't show inline info for Java output, since Java has no inline
      // concept.
      Config_getBool("INLINE_INFO") = FALSE;
   }

   int &depth = Config_getInt("MAX_DOT_GRAPH_DEPTH");
   if (depth == 0) {
      depth = 1000;
   }

   int &hue = Config_getInt("HTML_COLORSTYLE_HUE");
   if (hue < 0) {
      hue = 0;
   } else if (hue >= 360) {
      hue = hue % 360;
   }

   int &sat = Config_getInt("HTML_COLORSTYLE_SAT");
   if (sat < 0) {
      sat = 0;
   } else if (sat > 255) {
      sat = 255;
   }
   int &gamma = Config_getInt("HTML_COLORSTYLE_GAMMA");
   if (gamma < 40) {
      gamma = 40;
   } else if (gamma > 240) {
      gamma = 240;
   }

   QByteArray mathJaxFormat = Config_getEnum("MATHJAX_FORMAT");
   if (!mathJaxFormat.isEmpty() && mathJaxFormat != "HTML-CSS" &&
         mathJaxFormat != "NativeMML" && mathJaxFormat != "SVG") {
      err("Error: Unsupported value for MATHJAX_FORMAT: Should be one of HTML-CSS, NativeMML, or SVG\n");
      Config_getEnum("MATHJAX_FORMAT") = "HTML-CSS";
   }

   // add default words if needed
   QStringList &annotationFromBrief = Config_getList("ABBREVIATE_BRIEF");
   if (annotationFromBrief.isEmpty()) {
      annotationFromBrief.append("The $name class");
      annotationFromBrief.append("The $name widget");
      annotationFromBrief.append("The $name file");
      annotationFromBrief.append("is");
      annotationFromBrief.append("provides");
      annotationFromBrief.append("specifies");
      annotationFromBrief.append("contains");
      annotationFromBrief.append("represents");
      annotationFromBrief.append("a");
      annotationFromBrief.append("an");
      annotationFromBrief.append("the");
   }
   

   checkFileName("GENERATE_TAGFILE");

*/

}




/*

static void cleanUpPaths(QStringList &str)
{
   for (auto &sfp : str) {  

      if (sfp.isEmpty()) {
         continue;
      }

      sfp.replace("\\", "/");
    
      QByteArray path = sfp.toUtf8();

      if ((path.at(0) != '/' && (path.length() <= 2 || path.at(1) != ':')) || path.at(path.length() - 1) != '/' ) {

         QFileInfo fi(path);

         if (fi.exists() && fi.isDir()) {
            sfp = fi.absoluteFilePath() + "/";           
         }
      }      
   }
}

void Config::checkFileName(const char *optionName)
{
   QByteArray &s = Config_getString(optionName);
   QByteArray val = s.trimmed().toLower();

   if ((val == "yes" || val == "true"  || val == "1" || val == "all") ||
         (val == "no"  || val == "false" || val == "0" || val == "none")) {

      err("Error: file name expected for option %s, got %s instead. Ignoring...\n", optionName, s.data());
      s = ""; // note the use of &s above: this will change the option value!
   }
}

*/

