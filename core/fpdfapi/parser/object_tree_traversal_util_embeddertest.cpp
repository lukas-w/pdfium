// Copyright 2023 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fpdfapi/parser/object_tree_traversal_util.h"

#include <stdint.h>

#include <set>

#include "core/fpdfapi/parser/cpdf_dictionary.h"
#include "core/fpdfapi/parser/cpdf_document.h"
#include "core/fpdfapi/parser/cpdf_null.h"
#include "core/fpdfapi/parser/cpdf_parser.h"
#include "core/fpdfapi/parser/cpdf_reference.h"
#include "testing/embedder_test.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::ElementsAre;
using testing::UnorderedElementsAre;

using ObjectTreeTraversalUtilEmbedderTest = EmbedderTest;

namespace {

CPDF_Document* GetCPDFDocument(FPDF_DOCUMENT document) {
  // This is cheating slightly to avoid a layering violation, since this file
  // cannot include fpdfsdk/cpdfsdk_helpers.h to get access to
  // CPDFDocumentFromFPDFDocument().
  return reinterpret_cast<CPDF_Document*>((document));
}

}  // namespace

TEST_F(ObjectTreeTraversalUtilEmbedderTest, GetObjectsWithReferencesBasic) {
  ASSERT_TRUE(OpenDocument("hello_world.pdf"));
  CPDF_Document* doc = GetCPDFDocument(document());
  std::set<uint32_t> referenced_objects = GetObjectsWithReferences(doc);
  EXPECT_THAT(referenced_objects, UnorderedElementsAre(1, 2, 3, 4, 5, 6));
}

TEST_F(ObjectTreeTraversalUtilEmbedderTest, GetObjectsWithReferencesNewDoc) {
  ScopedFPDFDocument new_doc(FPDF_CreateNewDocument());
  CPDF_Document* doc = GetCPDFDocument(new_doc.get());
  std::set<uint32_t> referenced_objects = GetObjectsWithReferences(doc);
  // Empty documents have a catalog and an empty pages object.
  EXPECT_THAT(referenced_objects, UnorderedElementsAre(1, 2));
}

TEST_F(ObjectTreeTraversalUtilEmbedderTest,
       GetObjectsWithReferencesCircularRefs) {
  ASSERT_TRUE(OpenDocument("circular_viewer_ref.pdf"));
  CPDF_Document* doc = GetCPDFDocument(document());
  std::set<uint32_t> referenced_objects = GetObjectsWithReferences(doc);
  // The trailer points at a catalog, and the catalog only references itself.
  EXPECT_THAT(referenced_objects, UnorderedElementsAre(1));
}

TEST_F(ObjectTreeTraversalUtilEmbedderTest,
       GetObjectsWithReferencesCrossRefStream) {
  ASSERT_TRUE(OpenDocument("bug_1399.pdf"));
  CPDF_Document* doc = GetCPDFDocument(document());
  std::set<uint32_t> referenced_objects = GetObjectsWithReferences(doc);
  // The trailer is the dictionary inside /XRef object 16 0. Note that it
  // references object 3 0, but the rest of the document does not.
  EXPECT_THAT(referenced_objects,
              UnorderedElementsAre(1, 2, 3, 4, 5, 12, 13, 14, 16));
}

TEST_F(ObjectTreeTraversalUtilEmbedderTest,
       GetObjectsWithReferencesObjectZero) {
  ASSERT_TRUE(OpenDocument("rectangles_object_zero.pdf"));
  CPDF_Document* doc = GetCPDFDocument(document());
  std::set<uint32_t> referenced_objects = GetObjectsWithReferences(doc);
  EXPECT_THAT(referenced_objects, UnorderedElementsAre(1, 2, 3, 4));
}

TEST_F(ObjectTreeTraversalUtilEmbedderTest,
       GetObjectsWithMultipleReferencesBasic) {
  ASSERT_TRUE(OpenDocument("hello_world.pdf"));
  CPDF_Document* doc = GetCPDFDocument(document());
  std::set<uint32_t> referenced_objects = GetObjectsWithMultipleReferences(doc);
  EXPECT_TRUE(referenced_objects.empty());
}

TEST_F(ObjectTreeTraversalUtilEmbedderTest,
       GetObjectsWithMultipleReferencesNewDoc) {
  ScopedFPDFDocument new_doc(FPDF_CreateNewDocument());
  CPDF_Document* doc = GetCPDFDocument(new_doc.get());
  std::set<uint32_t> referenced_objects = GetObjectsWithMultipleReferences(doc);
  EXPECT_TRUE(referenced_objects.empty());
}

TEST_F(ObjectTreeTraversalUtilEmbedderTest,
       GetObjectsWithMultipleReferencesCircularRefs) {
  ASSERT_TRUE(OpenDocument("circular_viewer_ref.pdf"));
  CPDF_Document* doc = GetCPDFDocument(document());
  std::set<uint32_t> referenced_objects = GetObjectsWithMultipleReferences(doc);
  EXPECT_TRUE(referenced_objects.empty());
}

TEST_F(ObjectTreeTraversalUtilEmbedderTest,
       GetObjectsWithMultipleReferencesSharedObjects) {
  ASSERT_TRUE(OpenDocument("hello_world_2_pages.pdf"));
  CPDF_Document* doc = GetCPDFDocument(document());
  std::set<uint32_t> referenced_objects = GetObjectsWithMultipleReferences(doc);
  EXPECT_THAT(referenced_objects, UnorderedElementsAre(5, 6, 7));
}

TEST_F(ObjectTreeTraversalUtilEmbedderTest,
       GetObjectsWithMultipleReferencesObjectZero) {
  ASSERT_TRUE(OpenDocument("rectangles_object_zero.pdf"));
  CPDF_Document* doc = GetCPDFDocument(document());
  std::set<uint32_t> referenced_objects = GetObjectsWithMultipleReferences(doc);
  EXPECT_TRUE(referenced_objects.empty());
}

TEST_F(ObjectTreeTraversalUtilEmbedderTest,
       GetObjectsWithMultipleReferencesDAGCrossEdge) {
  ScopedFPDFDocument new_doc(FPDF_CreateNewDocument());
  CPDF_Document* doc = GetCPDFDocument(new_doc.get());

  RetainPtr<CPDF_Dictionary> root = doc->GetMutableRoot();
  ASSERT_TRUE(root);
  RetainPtr<CPDF_Dictionary> pages = root->GetMutableDictFor("Pages");
  ASSERT_TRUE(pages);

  auto obj4 = doc->NewIndirect<CPDF_Dictionary>();
  auto obj5 = doc->NewIndirect<CPDF_Dictionary>();
  auto obj6 = doc->NewIndirect<CPDF_Dictionary>();
  auto obj7 = doc->NewIndirect<CPDF_Dictionary>();

  const uint32_t obj4_num = obj4->GetObjNum();
  const uint32_t obj5_num = obj5->GetObjNum();
  const uint32_t obj6_num = obj6->GetObjNum();
  const uint32_t obj7_num = obj7->GetObjNum();

  ASSERT_EQ(4u, obj4_num);
  ASSERT_EQ(5u, obj5_num);
  ASSERT_EQ(6u, obj6_num);
  ASSERT_EQ(7u, obj7_num);

  pages->SetNewFor<CPDF_Reference>("A", doc, obj4_num);
  pages->SetNewFor<CPDF_Reference>("B", doc, obj5_num);
  obj4->SetNewFor<CPDF_Reference>("A", doc, obj6_num);
  obj5->SetNewFor<CPDF_Reference>("A", doc, obj7_num);
  obj5->SetNewFor<CPDF_Reference>("B", doc, obj4_num);

  // Verify the extracted reference graph before pruning.
  PdfReferenceGraph graph = BuildPdfReferenceGraphForTesting(doc);
  EXPECT_THAT(graph.roots, ElementsAre(1u));
  EXPECT_THAT(graph.adj[2], ElementsAre(obj4_num, obj5_num));
  EXPECT_THAT(graph.adj[obj4_num], ElementsAre(obj6_num));
  EXPECT_THAT(graph.adj[obj5_num], ElementsAre(obj7_num, obj4_num));

  // Verify the subsequent cycle pruning and multi-reference calculation.
  std::set<uint32_t> referenced_objects = GetObjectsWithMultipleReferences(doc);
  EXPECT_THAT(referenced_objects, UnorderedElementsAre(obj4_num));
}

TEST_F(ObjectTreeTraversalUtilEmbedderTest,
       GetObjectsWithReferencesTrailerWithoutRoot) {
  ASSERT_TRUE(OpenDocument("hello_world.pdf"));
  CPDF_Document* doc = GetCPDFDocument(document());
  doc->GetParser()->GetMutableTrailerForTesting()->SetNewFor<CPDF_Null>("Root");
  std::set<uint32_t> referenced_objects = GetObjectsWithReferences(doc);
  EXPECT_THAT(referenced_objects, UnorderedElementsAre(1, 2, 3, 4, 5, 6));
}

TEST_F(ObjectTreeTraversalUtilEmbedderTest,
       GetObjectsWithMultipleReferencesTrailerWithoutRoot) {
  ASSERT_TRUE(OpenDocument("hello_world_2_pages.pdf"));
  CPDF_Document* doc = GetCPDFDocument(document());
  doc->GetParser()->GetMutableTrailerForTesting()->SetNewFor<CPDF_Null>("Root");
  std::set<uint32_t> referenced_objects = GetObjectsWithMultipleReferences(doc);
  EXPECT_THAT(referenced_objects, UnorderedElementsAre(5, 6, 7));
}
