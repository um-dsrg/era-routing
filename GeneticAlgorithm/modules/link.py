class Link:
    """Class representing a Link

    Attributes
        id:       The link id

        cost:     The link's cost

        capacity: The link's capacity
    """

    def __init__(self, link_element):
        self.id = int(link_element.get('Id'))
        self.cost = float(link_element.get('Cost'))
        self.capacity = float(link_element.get('Capacity'))

    def __repr__(self):
        return 'Link(' + self.__str__() + ')'

    def __str__(self):
        return 'Link ID: {} Cost: {} Capacity: {}'.format(self.id, self.cost,
                                                          self.capacity)
