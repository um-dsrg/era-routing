#!/usr/bin/env python3

import numpy as np
import ga_helpers as gah


def main():

    gah.remove_excess([5, 5, 5], 15)

    while True:
        randomExcess = np.random.random_sample()
        randomChromosome = np.random.random_sample((1000, ))

        gah.remove_excess(randomChromosome, randomExcess)


if __name__ == "__main__":
    main()
