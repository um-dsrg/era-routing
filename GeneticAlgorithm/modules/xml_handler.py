from lxml import etree


class XmlHandler:
    """Class that handles XML file operations."""

    def __init__(self, xml_file_path: str, root_element_str: str = None):
        self.xml_path = xml_file_path

        if root_element_str is None:
            self.root_element = self._parse_xml_file()
        else:
            self.root_element = etree.Element(root_element_str)

    def get_root(self):
        """Return the XML root element."""
        if self.root_element is None:
            raise RuntimeError('XML Root element is None.')

        return self.root_element

    def save_xml_file(self):
        """Save the XML file."""
        tree = etree.ElementTree(self.root_element)
        with open(self.xml_path, 'wb') as output_file:
            tree.write(output_file, pretty_print=True, xml_declaration=True,
                       encoding="utf-8")

    def _parse_xml_file(self):
        """Parse the xml file and return the root node."""
        parser = etree.XMLParser(remove_blank_text=True)
        return etree.parse(self.xml_path, parser).getroot()
