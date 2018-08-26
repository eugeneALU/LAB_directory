# -----------------------------------------------------------------------------
# Distributed Systems (TDDD25)
# -----------------------------------------------------------------------------
# Author: Sergiu Rafiliu (sergiu.rafiliu@liu.se)
# Modified: 16 March 2017
#
# Copyright 2012-2017 Linkoping University
# -----------------------------------------------------------------------------

import threading
import socket
import json

"""Object Request Broker

This module implements the infrastructure needed to transparently create
objects that communicate via networks. This infrastructure consists of:

--  Strub ::
        Represents the image of a remote object on the local machine.
        Used to connect to remote objects. Also called Proxy.
--  Skeleton ::
        Used to listen to incoming connections and forward them to the
        main object. File "../modules/Common/orb.py", line 124, in __init__
s.

"""


class CommunicationError(Exception):
    pass


class Stub(object):

    """ Stub for generic objects distributed over the network.

    This is  wrapper object for a socket.

    """

    def __init__(self, address):
        self.address = tuple(address)

    def _rmi(self, method, *args):

        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

        try:
            self.socket.connect(self.address)
            worker = self.socket.makefile(mode="rw")

            message = {'method':method, 'args':args}
            data = json.dumps(message)
            worker.write(data + '\n')
            worker.flush()
            receive = json.loads(worker.readline())
            if ("result" in receive):
                return receive["result"]
            elif ("error" in receive):
                error = type(receive["error"]["name"],(Exception,),{})
                raise error(receive["error"]["args"])

        except Exception as e:
            print(">>>",(type(e).__name__), ", error args: ", e.args)
            if (method == "register"):
                return "error", "error"
            elif (method == "require_all"):
                return [["error",self.address]]

    def __getattr__(self, attr):
        """Forward call to name over the network at the given address."""
        def rmi_call(*args):        #args is tuple that consists of data we pass
            return self._rmi(attr, *args)
        return rmi_call


class Request(threading.Thread):

    """Run the incoming requests on the owner object of the skeleton."""

    def __init__(self, owner, conn, addr):
        threading.Thread.__init__(self)
        self.addr = addr
        self.conn = conn
        self.owner = owner
        self.daemon = True

    def run(self):

       try:
           worker = self.conn.makefile(mode="rw")
           request = json.loads(worker.readline())


           result = getattr(self.owner,request["method"])(*request["args"])


           result = json.dumps({"result" : result})
           worker.write(result + '\n')
           worker.flush()
       except Exception as e:
          print("error name: " + str(type(e).__name__) + " error args: "+ str(e.args))
       finally:
           self.conn.close()


class Skeleton(threading.Thread):

    """ Skeleton class for a generic owner.

    This is used to listen to an address of the network, manage incoming
    connections and forward calls to the generic owner class.

    """

    def __init__(self, owner, address):
        threading.Thread.__init__(self)
        self.address = address
        self.owner = owner
        self.daemon = True

        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.socket.bind(self.address)
        self.socket.listen(1)

    def run(self):

        while (1):
            try:
                conn, addr = self.socket.accept()
                req = Request(self.owner, conn, addr)
                req.start()
            except:
                print("connection error")

class Peer:

    """Class, extended by objects that communicate over the network."""

    def __init__(self, l_address, ns_address, ptype):
        self.type = ptype
        self.hash = ""
        self.id = -1
        self.address = self._get_external_interface(l_address)
        self.skeleton = Skeleton(self, self.address)
        self.name_service_address = self._get_external_interface(ns_address)
        self.name_service = Stub(self.name_service_address)

    # Private methods

    def _get_external_interface(self, address):
        """ Determine the external interface associated with a host name.

        This function translates the machine's host name into its the
        machine's external address, not into '127.0.0.1'.

        """

        addr_name = address[0]
        if addr_name != "":
            addrs = socket.gethostbyname_ex(addr_name)[2]
            if len(addrs) == 0:
                raise CommunicationError("Invalid address to listen to")
            elif len(addrs) == 1:
                addr_name = addrs[0]
            else:
                al = [a for a in addrs if a != "127.0.0.1"]
                addr_name = al[0]
        addr = list(address)
        addr[0] = addr_name
        return tuple(addr)

    # Public methods

    def start(self):
        """Start the communication interface."""

        self.skeleton.start()
        self.id, self.hash = self.name_service.register(self.type,
                                                        self.address)

    def destroy(self):
        """Unregister the object before removal."""

        self.name_service.unregister(self.id, self.type, self.hash)

    def check(self):
        """Checking to see if the object is still alive."""

        return (self.id, self.type)
