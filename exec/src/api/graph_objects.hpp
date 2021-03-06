
/**
 * @file
 * @author  Aapo Kyrola <akyrola@cs.cmu.edu>
 * @version 1.0
 *
 * @section LICENSE
 *
 * Copyright [2012] [Aapo Kyrola, Guy Blelloch, Carlos Guestrin / Carnegie Mellon University]
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 * http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 
 *
 * @section DESCRIPTION
 *
 * Vertex and Edge objects.
 */


#ifndef DEF_CE_Graph_OBJECTS
#define DEF_CE_Graph_OBJECTS

#include <vector>
#include <assert.h>
#include <omp.h>
#include <string.h>

#include "CE_Graph_types.hpp"
#include "util/qsort.hpp"

namespace CE_Graph {
    
/**
 * GNU COMPILER HACK TO PREVENT WARNINGS "Unused variable", if 
 * the particular app being compiled does not use a function.
 */
#ifdef __GNUC__
#define VARIABLE_IS_NOT_USED __attribute__ ((unused))
#else
#define VARIABLE_IS_NOT_USED
#endif
    
    
    
    template <typename EdgeDataType>
    class CE_Graph_edge {
        
    public:
        vid_t vertexid; // Source or Target vertex id. Clear from context.
        EdgeDataType * data_ptr;
        EdgeDataType * data_ptr_syn; //for for synchronization
        
        CE_Graph_edge() {}
        CE_Graph_edge(vid_t _vertexid, EdgeDataType * edata_ptr ,EdgeDataType * edata_ptr_syn) : vertexid(_vertexid), data_ptr(edata_ptr), data_ptr_syn(edata_ptr_syn) {
        }
        
#ifndef DYNAMICEDATA
        EdgeDataType get_data() {
            return * data_ptr;
        }
        
        void set_data(EdgeDataType x) {
            if(data_ptr_syn == NULL){
		 *data_ptr = x;
	    } else{ 
		*data_ptr_syn = x;
	   }
        }
#else 
        EdgeDataType * get_vector() {  // EdgeDataType is a chivector
            return data_ptr;
        }
#endif
        
        /**
          * Returns id of the endpoint of this edge. 
          */
        vid_t vertex_id() {
            return vertexid;
        }
        
 
    }  __attribute__((packed));
    
    template <typename ET>
    bool eptr_less(const CE_Graph_edge<ET> &a, const CE_Graph_edge<ET> &b) {
        return a.vertexid < b.vertexid;
    }
    
    
#ifdef SUPPORT_DELETIONS
    
    /*
     * Hacky support for edge deletions.
     * Edges are deleted by setting the value of the edge to a special
     * value that denotes it was deleted.
     * In the future, a better system could be designed.
     */
    
    // This is hacky...
    static inline bool VARIABLE_IS_NOT_USED is_deleted_edge_value(int val);
    static inline bool VARIABLE_IS_NOT_USED is_deleted_edge_value(bool val) {
        return val;
    }
    
    static inline bool VARIABLE_IS_NOT_USED is_deleted_edge_value(int val);
    static inline bool VARIABLE_IS_NOT_USED is_deleted_edge_value(int val) {
        return 0xffffffff == (unsigned int)val;
    }
    
    static inline bool VARIABLE_IS_NOT_USED is_deleted_edge_value(vid_t val);
    static inline bool VARIABLE_IS_NOT_USED is_deleted_edge_value(vid_t val) {
        return 0xffffffffu == val;
    }
    
    
    static inline bool VARIABLE_IS_NOT_USED is_deleted_edge_value(float val);
    static inline bool VARIABLE_IS_NOT_USED is_deleted_edge_value(float val) {
        return !(val < 0 || val > 0);
    }
    
    static void VARIABLE_IS_NOT_USED remove_edgev(CE_Graph_edge<bool> * e);
    static void VARIABLE_IS_NOT_USED remove_edgev(CE_Graph_edge<bool> * e) {
        e->set_data(true);
    }
    
    static void VARIABLE_IS_NOT_USED remove_edgev(CE_Graph_edge<vid_t> * e);
    static void VARIABLE_IS_NOT_USED remove_edgev(CE_Graph_edge<vid_t> * e) {
        e->set_data(0xffffffff);
    }
    
    static void VARIABLE_IS_NOT_USED remove_edgev(CE_Graph_edge<int> * e);
    static void VARIABLE_IS_NOT_USED remove_edgev(CE_Graph_edge<int> * e) {
        e->set_data(0xffffffff);
    }
    
#endif  
    
    
    template <typename VertexDataType, typename EdgeDataType>
    class internal_CE_Graph_vertex {
        
    public:   // Todo, use friend
        int inc;
        volatile int outc;
        
        vid_t vertexid;

    protected:
        CE_Graph_edge<EdgeDataType> * inedges_ptr;
        CE_Graph_edge<EdgeDataType> * outedges_ptr;
        
                
    public:
        bool modified;
        VertexDataType * dataptr;


        /* Accessed directly by the engine */
        bool scheduled;
        bool parallel_safe;
        bool crucial_flag;//for previous update
        
#ifdef SUPPORT_DELETIONS
        int deleted_inc;
        int deleted_outc;
#endif
        
        
        internal_CE_Graph_vertex() : inc(0), outc(0) {
#ifdef SUPPORT_DELETIONS
            deleted_outc = deleted_inc = 0;
#endif
            dataptr = NULL;
        }
        
        
        internal_CE_Graph_vertex(vid_t _id, CE_Graph_edge<EdgeDataType> * iptr, 
                                CE_Graph_edge<EdgeDataType> * optr, 
                                 int indeg, 
                                 int outdeg) : 
                            vertexid(_id), inedges_ptr(iptr), outedges_ptr(optr) {
            inc = 0;
            outc = 0;
            scheduled = false;
            modified = false;
            parallel_safe = true;
            crucial_flag = false; // for previous update
            dataptr = NULL;
#ifdef SUPPORT_DELETIONS
            deleted_inc = 0;
            deleted_outc = 0;
#endif
        }
        
        virtual ~internal_CE_Graph_vertex() {}

        
        vid_t id() const {
            return vertexid;
        }
        
        int num_inedges() const { 
            return inc; 
            
        }
        int num_outedges() const { 
            return outc; 
        }
        int num_edges() const { 
            return inc + outc; 
        }


        
                
        // Optimization: as only memshard (not streaming shard) creates inedgers,
        // we do not need atomic instructions here!
        inline void add_inedge(vid_t src, EdgeDataType * ptr,  EdgeDataType * ptr_syn, bool special_edge) {
#ifdef SUPPORT_DELETIONS
            if (inedges_ptr != NULL && is_deleted_edge_value(*ptr)) {
                deleted_inc++;
                return;
            }
#endif
            if (inedges_ptr != NULL) 
                inedges_ptr[inc] = CE_Graph_edge<EdgeDataType>(src, ptr, ptr_syn);
            inc++;  // Note: do not move inside the brackets, since we need to still keep track of inc even if inedgeptr is null!
            assert(src != vertexid);
          /*  if(inedges_ptr != NULL && inc > outedges_ptr - inedges_ptr) {
                logstream(LOG_FATAL) << "Tried to add more in-edges as the stored in-degree of this vertex (" << src << "). Perhaps a preprocessing step had failed?" << std::endl;
                assert(inc <= outedges_ptr - inedges_ptr);
            } */  // Deleted, since does not work when we have separate in-edge and out-edge arrays
        }
        
        inline void add_outedge(vid_t dst, EdgeDataType * ptr, EdgeDataType * ptr_syn, bool special_edge) {
#ifdef SUPPORT_DELETIONS
            if (outedges_ptr != NULL && is_deleted_edge_value(*ptr)) {
                deleted_outc++;
                return;
            }
#endif
            int i = __sync_add_and_fetch(&outc, 1);
            if (outedges_ptr != NULL) outedges_ptr[i-1] = CE_Graph_edge<EdgeDataType>(dst, ptr, ptr_syn);
            assert(dst != vertexid);
        }
        
        
    };
    
    template <typename VertexDataType, typename EdgeDataType >
    class CE_Graph_vertex : public internal_CE_Graph_vertex<VertexDataType, EdgeDataType> {
        
    public:
        
        CE_Graph_vertex() : internal_CE_Graph_vertex<VertexDataType, EdgeDataType>() { }
        
        CE_Graph_vertex(vid_t _id, 
                        CE_Graph_edge<EdgeDataType> * iptr, 
                                 CE_Graph_edge<EdgeDataType> * optr, 
                        int indeg, 
                        int outdeg) : 
            internal_CE_Graph_vertex<VertexDataType, EdgeDataType>(_id, iptr, optr, indeg, outdeg) {}
        
        virtual ~CE_Graph_vertex() {}
        
        /** 
          * Returns ith edge of a vertex, ignoring 
          * edge direction.
          */
        CE_Graph_edge<EdgeDataType> * edge(int i) {
            if (i < this->inc) return inedge(i);
            else return outedge(i - this->inc);
        }

        
        CE_Graph_edge<EdgeDataType> * inedge(int i) {
            assert(i >= 0 && i < this->inc);
            return &this->inedges_ptr[i];
        }
        
        CE_Graph_edge<EdgeDataType> * outedge(int i) {
            assert(i >= 0 && i < this->outc);
            return &this->outedges_ptr[i];
        }        
        
        CE_Graph_edge<EdgeDataType> * random_outedge() {
            if (this->outc == 0) return NULL;
            return outedge((int) (std::abs(random()) % this->outc));
        }
            
        /** 
          * Get the value of vertex
          */
#ifndef DYNAMICVERTEXDATA
        VertexDataType get_data() {
            return *(this->dataptr);
        }
#else
        // VertexDataType must be a chivector
        VertexDataType * get_vector() {
            this->modified = true;  // Assume vector always modified... Temporaryh solution.
            return this->dataptr;
        }
#endif
        
        /**
          * Modify the vertex value. The new value will be
          * stored on disk.
          */
        virtual void set_data(VertexDataType d) {
            *(this->dataptr) = d;
            this->modified = true;
        }
        
        // TODO: rethink
        static bool computational_edges() {
            return false;
        }
        static bool read_outedges() {
            return true;
        }
        
        
        /**
         * Sorts all the edges. Note: this will destroy information
         * about the in/out direction of an edge. Do use only if you
         * ignore the edge direction.
         */
        void VARIABLE_IS_NOT_USED sort_edges_indirect() {
            // Check for deleted edges first...
            if (this->inc != (this->outedges_ptr - this->inedges_ptr)) {
                // Moving
                memmove(&this->inedges_ptr[this->inc], this->outedges_ptr, this->outc * sizeof(CE_Graph_edge<EdgeDataType>));
                this->outedges_ptr = &this->inedges_ptr[this->inc];
            }
            quickSort(this->inedges_ptr, (int) (this->inc + this->outc), eptr_less<EdgeDataType>);
            
        }
        
        
#ifdef SUPPORT_DELETIONS
        void VARIABLE_IS_NOT_USED remove_edge(int i) {
            remove_edgev(edge(i));
        }
        
        void VARIABLE_IS_NOT_USED remove_inedge(int i) {
            remove_edgev(inedge(i));
        }
        
        void VARIABLE_IS_NOT_USED remove_outedge(int i) {
            remove_edgev(outedge(i));
        }
        
        void VARIABLE_IS_NOT_USED remove_alledges() {
            for(int j=this->num_edges()-1; j>=0; j--) remove_edge(j);
        }
        
        
        void VARIABLE_IS_NOT_USED remove_alloutedges() {
            for(int j=this->num_outedges()-1; j>=0; j--) remove_outedge(j);
        }
        
        void VARIABLE_IS_NOT_USED remove_allinedges() {
            for(int j=this->num_inedges()-1; j>=0; j--) remove_inedge(j);
        }
        
        
#endif
        
        
    };
    
    /**
      * Experimental code
      */
    
    // If highest order bit is set, the edge is "special". This is used
    // to indicate - in the neighborhood model - that neighbor's value is
    // cached in memory. 
#define HIGHMASK (1 + (2147483647 >> 1))
#define CLEARMASK (2147483647 >> 1)
    inline vid_t translate_edge(vid_t rawid, bool &is_special) {
        is_special = (rawid & HIGHMASK) != 0;
        return rawid & CLEARMASK;
    }
    inline vid_t make_special(vid_t rawid) {
        return rawid | HIGHMASK;
    }
    inline bool is_special(vid_t rawid) {
        return (rawid & HIGHMASK) != 0;
    }
    
    

} // Namespace

#endif
