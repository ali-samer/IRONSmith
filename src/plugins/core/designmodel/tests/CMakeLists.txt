include(IRONSmithFunctions)

ironsmith_define_tests(
        DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
        LINK_LIBS
            IRONSmith::DesignModelPlugin
        TEST DesignModel
            SOURCES
                tst_DesignModelMetadata.cpp
                tst_DesignModelDocument.cpp
                tst_DesignModelMutations.cpp
                tst_DesignModelIndex.cpp
                tst_DesignModelExtras.cpp
)
