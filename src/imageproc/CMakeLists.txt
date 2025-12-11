PROJECT(imageproc)

SET(
        sources
        Constants.h Constants.cpp
        BinaryImage.cpp BinaryImage.h
        BinaryThreshold.cpp BinaryThreshold.h
        SlicedHistogram.cpp SlicedHistogram.h
        ByteOrder.h BWColor.h
        ConnComp.h Connectivity.h
        BitOps.cpp BitOps.h
        SeedFill.cpp SeedFill.h
        ConnCompEraser.cpp ConnCompEraser.h
        ConnCompEraserExt.cpp ConnCompEraserExt.h
        GrayImage.cpp GrayImage.h
        Grayscale.cpp Grayscale.h
        RasterOp.h GrayRasterOp.h RasterOpGeneric.h
        UpscaleIntegerTimes.cpp UpscaleIntegerTimes.h
        ReduceThreshold.cpp ReduceThreshold.h
        Shear.cpp Shear.h
        SkewFinder.cpp SkewFinder.h
        OrthogonalRotation.cpp OrthogonalRotation.h
        Scale.cpp Scale.h
        Transform.cpp Transform.h
        Morphology.cpp Morphology.h
        IntegralImage.h
        Binarize.cpp Binarize.h
        PolygonUtils.cpp PolygonUtils.h
        PolygonRasterizer.cpp PolygonRasterizer.h
        HoughLineDetector.cpp HoughLineDetector.h
        GaussBlur.cpp GaussBlur.h
        Sobel.h
        MorphGradientDetect.cpp MorphGradientDetect.h
        PolynomialLine.cpp PolynomialLine.h
        PolynomialSurface.cpp PolynomialSurface.h
        SavGolKernel.cpp SavGolKernel.h
        SavGolFilter.cpp SavGolFilter.h
        DrawOver.cpp DrawOver.h
        AdjustBrightness.cpp AdjustBrightness.h
        SEDM.cpp SEDM.h
        ConnectivityMap.cpp ConnectivityMap.h
        InfluenceMap.cpp InfluenceMap.h
        MaxWhitespaceFinder.cpp MaxWhitespaceFinder.h
        RastLineFinder.cpp RastLineFinder.h
        ColorInterpolation.cpp ColorInterpolation.h
        LocalMinMaxGeneric.h
        SeedFillGeneric.cpp SeedFillGeneric.h
        FindPeaksGeneric.h
        ColorMixer.h
        ColorForId.h
        SIMDUtils.cpp SIMDUtils.h
)

# GPU acceleration sources (CUDA)
SET(gpu_sources
        gpu/CUDAUtils.h
)

# Add CUDA sources if CUDA is enabled, otherwise use stub implementations
IF(CUDA_ENABLED)
        SET(gpu_sources ${gpu_sources}
                gpu/CUDAUtils.cu
        )
        # Set CUDA-specific properties
        SET_SOURCE_FILES_PROPERTIES(gpu/CUDAUtils.cu PROPERTIES LANGUAGE CUDA)
ELSE()
        # Use stub implementation when CUDA is not available
        SET(gpu_sources ${gpu_sources}
                gpu/CUDAStubs.cpp
        )
ENDIF()

SOURCE_GROUP(Sources FILES ${sources})
SOURCE_GROUP(GPU FILES ${gpu_sources})

ADD_LIBRARY(imageproc STATIC ${sources} ${gpu_sources})
TARGET_LINK_LIBRARIES(imageproc Qt5::Core Qt5::Gui)

# Link CUDA runtime if available
IF(CUDA_ENABLED)
        TARGET_LINK_LIBRARIES(imageproc CUDA::cudart)
        TARGET_INCLUDE_DIRECTORIES(imageproc PRIVATE ${CUDAToolkit_INCLUDE_DIRS})
ENDIF()

ADD_SUBDIRECTORY(tests)
