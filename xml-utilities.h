#include <tinyxml2.h>
#include <string>

class XmlUtilities
{
public:
  /**
   *  \brief Returns the root node of the XML file
   *  \param xmlDoc A pointer to the XML document
   */
  static tinyxml2::XMLNode* GetRootNode (tinyxml2::XMLDocument* xmlDoc);

  /**
   *  \brief Inserts the root node.
   *  \param xmlDoc The XML Document
   *  \param rootElementName The name given to the root element
   *  \return Pointer to the newly created root node.
   */
  static tinyxml2::XMLNode* InsertRootNode (tinyxml2::XMLDocument* xmlDoc,
                                            const std::string& rootElementName);

  /**
   *  \brief Insert a time stamp in the Root Element
   *  \param xmlDoc The XML Document
   */
  static void InsertTimeStampInRootElement (const tinyxml2::XMLDocument* xmlDoc);
  /**
   *  \brief Save the XML file and add XML Declaration
   *
   *  Save the XML file and add a declaration if the insertDecleration variable is set to true.
   *  If the file cannot be saved an std::ios_base::failure exception will be thrown.
   *
   *  \param fileFullPath The full path, including the file name, where to save the XML log file
   *  \param xmlDoc The XML Document to save
   *  \param insertDecleration A boolean flag that determines whether an XML declaration is added
   *                           before saving the file.
   */
  static void SaveXmlFile (const std::string& fileFullPath, tinyxml2::XMLDocument* xmlDoc,
                           bool insertDeclaration = true);
private:
  XmlUtilities();
};
