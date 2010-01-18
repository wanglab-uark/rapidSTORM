#ifndef DSTORM_INPUT_SLOT_H
#define DSTORM_INPUT_SLOT_H

#include <dStorm/helpers/thread.h>
#include <cassert>
#include <boost/utility.hpp>

#include "Source.h"
#include "Slot_decl.h"

namespace dStorm {
namespace input {
    /** The Claim class represents a working license for an image.
     *  A working license is both a mutex (only a thread which possesses
     *  the corresponding claim may get a reference to an image) 
     *  and a status monitor; any image that had a claim on it is 
     *  regarded as finished. */
    template <typename T> class Claim {
      private:
        friend class Slot<T>;
        /** If this Claim is valid, this pointer points to the slot
         *  the Claim is for */
        Slot<T> *work;
        /** A Slot may call this claim constructor if its state allows. */
        Claim(Slot<T> *w) : work(w) { if (work) work->claims++; }

      public:
        Claim( const Claim<T>& o ) : work(o.work) {
            if ( work ) {
                ost::MutexLock lock( work->dataMutex );
                work->claims++;
            }
        }
        Claim<T>& operator=( const Claim<T>& o ) {
            if ( &o != this ) {
                this->~Claim();
                new (this) Claim(o);
            }
        }

        /** Forfeit claim and mark the corresponding image as Finished.
         *  To avoid finishing the image, reject() the Claim. */
        ~Claim() { if (work) work->finish(); }

        /** Returns true if access to this claim is allowed (the claim
         *  is termed good then). */
        bool isGood() { return work != NULL; }
        /** Returns true if an error occured for this object. */
        bool hasErrors() 
            { return isGood() && work->data.get() == NULL; }
        /** Return the base image for this claim. May only be called
         *  for good and error-free claims.
         *  \sa isGood()
         *  \sa hasErrors()
         **/
        T &operator*() { 
            assert(isGood() && work->data.get() != NULL);
            return *work->data;
        }
        /** Rejects the claim. Invalidates it and allows other 
         *  Claims on the same object. */
        void reject() { if (work) { work->abort(); work = NULL; } }

        int index() const { return work->my_index; }
    };

    /** A Slot is a buffer and state monitor for a single object.
     *  The Slot is supplied with a Source that is pulled on
     *  demand (if pullable) or waited upon if pushing.
     *
     *  An object wishing to access the object in a Slot should
     *  acquire a Claim with claim() and dereference that Claim.
     *  */
    template <typename T> class Slot {
      public:
        enum State { Untouched, /**< Data not yet present */
                     Delivered, /**< Data are present */
                     Processed, /**< Data are present 
                                     and have been claimed */
                     Discarded  /**< Data not present any more */
        };

        /** Constructor.
         *  \param src   Object Source. Must be provided, even if the
         *               Source pushes only.
         *  \param index The index is the object identifier for the
         *               Source. If -1 is given here, the index may
         *               be set later with setIndex().
         *  \param discardable A reference on the discarding license
         *                     in effect for this Slot (see Buffer for
         *                     that license). If this license changes,
         *                     call discard() in addition. */
        Slot(Source<T> &src, int index, const bool& discardable)
;
        /** The copy constructor. Does not copy the contained object,
         *  if already fetched. */
        Slot(const Slot<T> &c);
        /** The copy operator. Does not copy the contained object,
         *  if already fetched. */
        Slot& operator=(const Slot &c);
        ~Slot();

        /** Deferred index set (see constructor). */
        inline void setIndex(int i) 
            { if (my_index < 0) my_index = i; }

        /** Get a Claim on this object to work with it. */
        inline Claim<T> claim() 
            { return ( state >= Processed ) ? Claim<T>(NULL) : _workOn(); }
        /** Reset the state of this object. Does not discard an already
         *  fetched object, but sets the state to Untouched. */
        void clean();
        /** If allowed to, discard a fetched object. */
        bool discard();

        /** Supply the object for this slot. Counts as fetched from
         *  then on.
         *
         *  \param image Signals error if NULL. */
        void insert(std::auto_ptr< T > image);

        int index() const { return my_index; }

      private:
        friend class Claim<T>;
        Source<T> *source;
        int my_index, claims;
        const bool& mayDiscard;

        std::auto_ptr< T > data;
        State state;
        ost::Mutex dataMutex;
        ost::Condition gotData;

        void fetchData();
        Claim<T> _workOn();
        void finish();
    };
}
}
#endif
