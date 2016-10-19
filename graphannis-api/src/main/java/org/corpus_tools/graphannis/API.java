// Targeted by JavaCPP version 1.2.2: DO NOT EDIT THIS FILE

package org.corpus_tools.graphannis;

import java.nio.*;
import org.bytedeco.javacpp.*;
import org.bytedeco.javacpp.annotation.*;

public class API extends org.corpus_tools.graphannis.info.AnnisApiInfo {
    static { Loader.load(); }

@Name("std::vector<std::string>") public static class StringVector extends Pointer {
    static { Loader.load(); }
    /** Pointer cast constructor. Invokes {@link Pointer#Pointer(Pointer)}. */
    public StringVector(Pointer p) { super(p); }
    public StringVector(BytePointer ... array) { this(array.length); put(array); }
    public StringVector(String ... array) { this(array.length); put(array); }
    public StringVector()       { allocate();  }
    public StringVector(long n) { allocate(n); }
    private native void allocate();
    private native void allocate(@Cast("size_t") long n);
    public native @Name("operator=") @ByRef StringVector put(@ByRef StringVector x);

    public native long size();
    public native void resize(@Cast("size_t") long n);

    @Index public native @StdString BytePointer get(@Cast("size_t") long i);
    public native StringVector put(@Cast("size_t") long i, BytePointer value);
    @ValueSetter @Index public native StringVector put(@Cast("size_t") long i, @StdString String value);

    public StringVector put(BytePointer ... array) {
        if (size() != array.length) { resize(array.length); }
        for (int i = 0; i < array.length; i++) {
            put(i, array[i]);
        }
        return this;
    }

    public StringVector put(String ... array) {
        if (size() != array.length) { resize(array.length); }
        for (int i = 0; i < array.length; i++) {
            put(i, array[i]);
        }
        return this;
    }
}

// Parsed from annis/api/search.h

// #pragma once

// #include <memory>
// #include <vector>
// #include <list>

// #include <annis/db.h>
// #include <annis/dbcache.h>
// #include <annis/json/jsonqueryparser.h>
/**
 * An API for searching in a corpus.
 */
@Namespace("annis::api") @NoOffset public static class Search extends Pointer {
    static { Loader.load(); }
    /** Pointer cast constructor. Invokes {@link Pointer#Pointer(Pointer)}. */
    public Search(Pointer p) { super(p); }


  public static class CountResult extends Pointer {
      static { Loader.load(); }
      /** Default native constructor. */
      public CountResult() { super((Pointer)null); allocate(); }
      /** Native array allocator. Access with {@link Pointer#position(long)}. */
      public CountResult(long size) { super((Pointer)null); allocateArray(size); }
      /** Pointer cast constructor. Invokes {@link Pointer#Pointer(Pointer)}. */
      public CountResult(Pointer p) { super(p); }
      private native void allocate();
      private native void allocateArray(long size);
      @Override public CountResult position(long position) {
          return (CountResult)super.position(position);
      }
  
    public native long matchCount(); public native CountResult matchCount(long matchCount);
    public native long documentCount(); public native CountResult documentCount(long documentCount);
  }

  public Search(@StdString BytePointer databaseDir) { super((Pointer)null); allocate(databaseDir); }
  private native void allocate(@StdString BytePointer databaseDir);
  public Search(@StdString String databaseDir) { super((Pointer)null); allocate(databaseDir); }
  private native void allocate(@StdString String databaseDir);

  /**
   * Count all occurrences of an AQL query in a single corpus.
   *
   * @param corpus
   * @param queryAsJSON
   * @return
   */
  public native long count(@ByVal StringVector corpora,
                    @StdString BytePointer queryAsJSON);
  public native long count(@ByVal StringVector corpora,
                    @StdString String queryAsJSON);


  /**
   * Count all occurrences of an AQL query in a single corpus.
   *
   * @param corpus
   * @param queryAsJSON
   * @return
   */
  public native @ByVal CountResult countExtra(@ByVal StringVector corpora,
                    @StdString BytePointer queryAsJSON);
  public native @ByVal CountResult countExtra(@ByVal StringVector corpora,
                    @StdString String queryAsJSON);


  /**
   * Find occurrences of an AQL query in a single corpus.
   * @param corpora
   * @param queryAsJSON
   * @param offset
   * @param limit
   * @return
   */
  public native @ByVal StringVector find(@ByVal StringVector corpora, @StdString BytePointer queryAsJSON, long offset/*=0*/, long limit/*=10*/);
  public native @ByVal StringVector find(@ByVal StringVector corpora, @StdString BytePointer queryAsJSON);
  public native @ByVal StringVector find(@ByVal StringVector corpora, @StdString String queryAsJSON, long offset/*=0*/, long limit/*=10*/);
  public native @ByVal StringVector find(@ByVal StringVector corpora, @StdString String queryAsJSON);
}

 // end namespace annis


// Parsed from annis/api/admin.h

// #pragma once

// #include <string>
  @Namespace("annis::api") public static class Admin extends Pointer {
      static { Loader.load(); }
      /** Pointer cast constructor. Invokes {@link Pointer#Pointer(Pointer)}. */
      public Admin(Pointer p) { super(p); }
      /** Native array allocator. Access with {@link Pointer#position(long)}. */
      public Admin(long size) { super((Pointer)null); allocateArray(size); }
      private native void allocateArray(long size);
      @Override public Admin position(long position) {
          return (Admin)super.position(position);
      }
  
    public Admin() { super((Pointer)null); allocate(); }
    private native void allocate();

    /**
    * Imports data in the relANNIS format to the internal format used by graphANNIS.
    * @param sourceFolder
    * @param targetFolder
    */
   public static native void importRelANNIS(@StdString BytePointer sourceFolder, @StdString BytePointer targetFolder);
   public static native void importRelANNIS(@StdString String sourceFolder, @StdString String targetFolder);
  }
 // end namespace annis::api


}