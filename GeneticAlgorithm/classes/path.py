class Path:
    """Class representing a Path.

    Attributes
        id:    The path id.

        cost:  The paths's cost.

        links: List of links the path uses.
    """

    def __init__(self, path_element):
        self.id = int(path_element.get('Id'))
        self.cost = float(path_element.get('Cost'))
        self.links = [int(link_element.get('Id')) for link_element
                      in path_element.findall('Link')]

    def __repr__(self):
        return 'Path(' + self.__str__() + ')'

    def __str__(self):
        return ('Path ID: {} Path Cost: {} Links used: {}'.format(self.id,
                                                                  self.cost,
                                                                  self.links))
