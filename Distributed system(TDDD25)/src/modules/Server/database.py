# -----------------------------------------------------------------------------
# Distributed Systems (TDDD25)
# -----------------------------------------------------------------------------
# Author: Sergiu Rafiliu (sergiu.rafiliu@liu.se)
# Modified: 24 July 2013
#
# Copyright 2012 Linkoping University
# -----------------------------------------------------------------------------

"""Implementation of a simple database class."""

import random


class Database(object):

    """Class containing a database implementation."""

    def __init__(self, db_file):
        self.db_file = db_file
        self.rand = random.Random()
        self.rand.seed()
        self.word = []

        f = open(self.db_file)
        temp = f.readlines()
        store = ''
        for line in temp:
            line = line[:-1] #drop the \n at the end
            if line != '%':
                store += line
            else:
                self.word.append(store)
                store = ''

        del temp
        del store
        f.close()

    def read(self):
        """Read a random location in the database."""

        if (len(self.word) > 0):
            return self.word[self.rand.randint(0,len(self.word) - 1)]
        else:
            return "database is empty"

    def write(self, fortune):
        """Write a new fortune to the database."""

        f = open(self.db_file, 'a')
        f.write(fortune + '\n%\n')
        f.close()
        self.word.append(fortune)
