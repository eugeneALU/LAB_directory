# -----------------------------------------------------------------------------
# Distributed Systems (TDDD25)
# -----------------------------------------------------------------------------
# Author: Sergiu Rafiliu (sergiu.rafiliu@liu.se)
# Modified: 31 July 2013
#
# Copyright 2012 Linkoping University
# -----------------------------------------------------------------------------

"""Module for the distributed mutual exclusion implementation.

This implementation is based on the second Ricart-Agrawala algorithm.
The implementation should satisfy the following requests:
    --  when starting, the peer with the smallest id in the peer list
        should get the token.
    --  access to the state of each peer (dictionaries: request, token,
        and peer_list) should be protected.
    --  the implementation should graciously handle situations when a
        peer dies unexpectedly. All exceptions coming from calling
        peers that have died, should be handled such as the rest of the
        peers in the system are still working. Whenever a peer has been
        detected as dead, the token, request, and peer_list
        dictionaries should be updated accordingly.
    --  when the peer that has the token (either TOKEN_PRESENT or
        TOKEN_HELD) quits, it should pass the token to some other peer.
    --  For simplicity, we shall not handle the case when the peer
        holding the token dies unexpectedly.

"""

NO_TOKEN = 0
TOKEN_PRESENT = 1
TOKEN_HELD = 2


class DistributedLock(object):

    """Implementation of distributed mutual exclusion for a list of peers.

    Public methods:
        --  __init__(owner, peer_list)
        --  initialize()
        --  destroy()
        --  register_peer(pid)
        --  unregister_peer(pid)
        --  acquire()
        --  release()
        --  request_token(time, pid)
        --  obtain_token(token)
        --  display_status()

    """

    def __init__(self, owner, peer_list):
        self.peer_list = peer_list
        self.owner = owner
        self.time = 0
        self.token = None
        self.request = {}
        self.state = NO_TOKEN

    def _prepare(self, token):
        """Prepare the token to be sent as a JSON message.

        This step is necessary because in the JSON standard, the key to
        a dictionary must be a string whild in the token the key is
        integer.
        """
        return list(token.items())

    def _unprepare(self, token):
        """The reverse operation to the one above."""
        return dict(token)

    # Public methods

    def initialize(self):
        """ Initialize the state, request, and token dicts of the lock.

        Since the state of the distributed lock is linked with the
        number of peers among which the lock is distributed, we can
        utilize the lock of peer_list to protect the state of the
        distributed lock (strongly suggested).

        NOTE: peer_list must already be populated when this
        function is called.

        """
        self.peer_list.lock.acquire()
        self.token = {}
        is_smallest= True
        try:

            self.token[self.owner.id] = 0
            self.request[self.owner.id] = 0
            for pid, peer in self.peer_list.peers.items():
                self.token[pid] = 0
                self.request[pid] = 0
                #peer.register_peer(self.owner.id,self.owner.address)
                if (pid < self.owner.id):
                    # not smallest
                    is_smallest = False

            if is_smallest:
                self.state = TOKEN_PRESENT
        finally:
            self.peer_list.lock.release()

    def destroy(self):
        """ The object is being destroyed.

        If we have the token (TOKEN_PRESENT or TOKEN_HELD), we must
        give it to someone else.

        """
        # if have token
        self.peer_list.lock.acquire()
        try:
            if self.state in (TOKEN_HELD, TOKEN_PRESENT):
                #give token to someone else need it
                peers = sorted(self.peer_list.peers.items())
                for pid, peer in peers:
                    if (pid > self.owner.id) and (self.request[pid] > self.token[pid]):
                        self.token[self.owner.id] = self.time
                        try:
                            peer.obtain_token(self._prepare(self.token))
                            self.state = NO_TOKEN
                        except:
                            continue
                        return
                for pid, peer in peers:
                    if (pid < self.owner.id) and (self.request[pid] > self.token[pid]):
                        self.token[self.owner.id] = self.time
                        try:
                            peer.obtain_token(self._prepare(self.token))
                            self.state = NO_TOKEN
                        except:
                            continue
                        return
                # if no one need it give to anyone avilable
                for pid, peer in peers:
                    if (pid != self.owner.id):
                        try:
                            peer.unregister_peer(self.owner.id)
                            peer.obtain_token(self._prepare(self.token))
                        except:
                            continue
                        return
        finally:
            self.peer_list.lock.release()


        '''peer_list is a copy or refernce? '''
        # unrigister this peer
        #for pid, peer in self.peer_list.peers.items():
        #    peer.unregister_peer(self.owner.id)

    def register_peer(self, pid):
        """Called when a new peer joins the system."""

        self.peer_list.lock.acquire()

        try:
            self.request[pid] = 0
            self.token[pid] = 0

        finally:
            self.peer_list.lock.release()

    def unregister_peer(self, pid):
        """Called when a peer leaves the system."""

        self.peer_list.lock.acquire()

        try:
            del self.request[pid]
            if self.state in (TOKEN_HELD, TOKEN_PRESENT):
                # if have token, del token slot of this pid
                del self.token[pid]

        finally:
            self.peer_list.lock.release()

    def acquire(self):
        """Called when this object tries to acquire the lock."""
        print("Trying to acquire the lock...")
        self.peer_list.lock.acquire()

        try:
            if (self.state == NO_TOKEN):
                self.time += 1
                self.request[self.owner.id] = self.time
                locallist = self.peer_list.peers.items()
                self.peer_list.lock.release()
                for pid, peer in locallist:
                    try:
                        peer.request_token(self.time, self.owner.id)
                    except:
                        continue
                self.peer_list.lock.acquire()
            elif (self.state == TOKEN_PRESENT):
                print("Already has token")
                self.state = TOKEN_HELD
            else:
                print("Right now using")

        finally:
            self.peer_list.lock.release()
            while (1):
                self.peer_list.lock.acquire()
                if (self.state == TOKEN_PRESENT):
                    self.state = TOKEN_HELD
                    self.peer_list.lock.release()
                    break
                self.peer_list.lock.release()

    def release(self):
        """Called when this object releases the lock."""
        print("Releasing the lock...")
        self.peer_list.lock.acquire()

        try:
            if (self.state == TOKEN_HELD):
                self.state = TOKEN_PRESENT
                self.time += 1

                # try to find peer that need token
                peers = sorted(self.peer_list.peers.items())
                for peerid, peer in peers:
                    if (peerid > self.owner.id) and (self.request[peerid] > self.token[peerid]):
                        self.token[self.owner.id] = self.time
                        try:
                            peer.obtain_token(self._prepare(self.token))
                            self.state = NO_TOKEN
                        except:
                            continue
                        return
                for peerid, peer in peers:
                    if (peerid < self.owner.id) and (self.request[peerid] > self.token[peerid]):
                        self.token[self.owner.id] = self.time
                        try:
                            peer.obtain_token(self._prepare(self.token))
                            self.state = NO_TOKEN
                        except:
                            continue
                        return
            else:
                print("No token to be released")
        finally:
            self.peer_list.lock.release()



    def request_token(self, time, pid):
        """Called when some other object requests the token from us."""
        self.peer_list.lock.acquire()
        self.time += 1

        try:
            self.request[pid] = max(time, self.time)

            if (self.state == TOKEN_PRESENT):
                peers = sorted(self.peer_list.peers.items())
                for peerid, peer in peers:
                    if (peerid > self.owner.id) and (self.request[peerid] > self.token[peerid]):
                        self.token[self.owner.id] = self.time
                        try:
                            peer.obtain_token(self._prepare(self.token))
                            self.state = NO_TOKEN
                        except:
                            continue
                        return
                for peerid, peer in peers:
                    if (peerid < self.owner.id) and (self.request[peerid] > self.token[peerid]):
                        self.token[self.owner.id] = self.time
                        try:
                            peer.obtain_token(self._prepare(self.token))
                            self.state = NO_TOKEN
                        except:
                            continue
                        return
            else:
                print("NO token to give out or right now using")
        finally:
            self.peer_list.lock.release()


    def obtain_token(self, token):
        """Called when some other object is giving us the token."""
        print("Receiving the token...")
        self.peer_list.lock.acquire()
        self.time += 1

        try:
            self.token = self._unprepare(token)
            if (self.request[self.owner.id] > self.token[self.owner.id]):
                # need token
                self.state = TOKEN_PRESENT
            else:
                # don't need token
                self.state = TOKEN_PRESENT
        except Exception as e:
            print(type(e),">>>",e.args)
        finally:
            self.peer_list.lock.release()

    def display_status(self):
        """Print the status of this peer."""
        self.peer_list.lock.acquire()
        try:
            nt = self.state == NO_TOKEN
            tp = self.state == TOKEN_PRESENT
            th = self.state == TOKEN_HELD
            print("State   :: no token      : {0}".format(nt))
            print("           token present : {0}".format(tp))
            print("           token held    : {0}".format(th))
            print("Request :: {0}".format(self.request))
            print("Token   :: {0}".format(self.token))
            print("Time    :: {0}".format(self.time))
        finally:
            self.peer_list.lock.release()
