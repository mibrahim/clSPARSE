/* ************************************************************************
 * Copyright 2015 Advanced Micro Devices, Inc.
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
 * ************************************************************************ */


#include "csr-meta.hpp"

#include "include/clSPARSE-private.hpp"
#include "internal/clsparse-control.hpp"

clsparseStatus
clsparseCsrMetaSize( clsparseCsrMatrix* csrMatx, clsparseControl control )
{
    clsparseCsrMatrixPrivate* pCsrMatx = static_cast<clsparseCsrMatrixPrivate*>( csrMatx );
    matrix_meta* meta_ptr = static_cast< matrix_meta* >( pCsrMatx->meta );

    clMemRAII< clsparseIdx_t > rCsrRowOffsets( control->queue( ), pCsrMatx->rowOffsets );
    clsparseIdx_t* rowDelimiters = rCsrRowOffsets.clMapMem( CL_TRUE, CL_MAP_READ, pCsrMatx->rowOffOffset( ), pCsrMatx->num_rows + 1 );
    meta_ptr->rowBlockSize = ComputeRowBlocksSize( rowDelimiters, pCsrMatx->num_rows, BLKSIZE, BLOCK_MULTIPLIER, ROWS_FOR_VECTOR );

    return clsparseSuccess;
}

clsparseStatus
clsparseCsrMetaCompute( clsparseCsrMatrix* csrMatx, clsparseControl control )
{
    clsparseCsrMatrixPrivate* pCsrMatx = static_cast<clsparseCsrMatrixPrivate*>( csrMatx );
    matrix_meta* meta_ptr = static_cast< matrix_meta* >( pCsrMatx->meta );

    // Check to ensure nRows can fit in 32 bits
    if( static_cast<cl_ulong>( pCsrMatx->num_rows ) > static_cast<cl_ulong>( pow( 2, ( 64 - ROW_BITS ) ) ) )
    {
        printf( "Number of Rows in the Sparse Matrix is greater than what is supported at present ((64-WG_BITS) bits) !" );
        return clsparseOutOfResources;
    }

    if( meta_ptr == nullptr )
    {
        pCsrMatx->meta = new matrix_meta;
        clsparseCsrMetaSize( csrMatx, control );
    }

    if( pCsrMatx->meta )
    {
        clMemRAII< clsparseIdx_t > rCsrRowOffsets( control->queue( ), pCsrMatx->rowOffsets );
        clsparseIdx_t* rowDelimiters = rCsrRowOffsets.clMapMem( CL_TRUE, CL_MAP_READ, pCsrMatx->rowOffOffset( ), pCsrMatx->num_rows + 1 );

        cl_ulong* ulCsrRowBlocks = static_cast< cl_ulong* >( control->queue.enqueueMapBuffer( meta_ptr->rowBlocks, CL_TRUE, CL_MAP_WRITE_INVALIDATE_REGION, meta_ptr->offRowBlocks, meta_ptr->rowBlockSize ) );

        ComputeRowBlocks( ulCsrRowBlocks, meta_ptr->rowBlockSize, rowDelimiters, pCsrMatx->num_rows, BLKSIZE, BLOCK_MULTIPLIER, ROWS_FOR_VECTOR, true );
        control->queue.enqueueUnmapMemObject( meta_ptr->rowBlocks, ulCsrRowBlocks );
    }
    return clsparseSuccess;
}

clsparseStatus
clsparseCsrMetaDelete( clsparseCsrMatrix* csrMatx )
{
    clsparseCsrMatrixPrivate* pCsrMatx = static_cast<clsparseCsrMatrixPrivate*>( csrMatx );

    if( pCsrMatx->meta == nullptr )
    {
        return clsparseSuccess;
    }

    matrix_meta* meta_ptr = static_cast< matrix_meta* >( pCsrMatx->meta );
    delete meta_ptr;
    pCsrMatx->meta = nullptr;

    return clsparseSuccess;
}
