//
// File:        rm_manager.cc
// Description: RM_Manager class implementation
// Authros:     Wilson
//

#include <cstdio>
#include <unistd.h>
#include <fcntl.h>
#include "pf.h"
#include "rm_internal.h"

//
// RM_Manager 
//
// Desc: Constructor - intended to be called once at begin of program
//       Handles creation, deletion, opening and closing of files.
//       It is associated with a PF_Manager that manages the page file.
//
RM_Manager::RM_Manager(PF_Manager &pfm)
{
    this->m_pfm = pfm;
}

// RM_Manager
//
// Desc: Destructor - callded once at the end of the program
//       Nothing need to be done here.
//
RM_Manager::~RM_Manager()
{
}

// CreateFile
//
// Desc: Create a record file named 'fileName' with record size 
//       of 'recordSize'.
//
RC RM_Manager::CreateFile(const char *fileName, int recordSize)
{
    RC rc, rc2;
    PF_FileHandle pfh;
    PF_PageHandle pph;
    PageNum pageNum;
    char *pData;
    RM_FileHdr *fileHdr;

    if (recordSize > PF_PAGE_SIZE) 
        return RM_OVERSIZE;

    // Create page file for record manager
    if ((rc = m_pfm.CreateFile(fileName)) && 
            (rc = m_pfm.OpenFile(fileName, pfh)))
        return (rc);

    // AllocatePage for file header
    if ((rc = pfh.AllocatePage(pph))) {
        m_pfm.CloseFile(pfh);
        return (rc);
    }

    // Get PageNum and pData
    if ((rc = pph.GetPageNum(pageNum)) &&
            (rc = pph.GetData(pData))) {
        goto err;
   }

    // Initialize file header
    fileHdr = (RM_FileHdr*)pData;
    fileHdr->recordSize = recordSize; 

    // MarkDirty ,UnpinPage and CloseFile
    if ((rc = pfh.MarkDirty(pageNum)) &&
            (rc = pfh.UnpinPage(pageNum)) &&
            (rc = pfh.FlushPages()) &&
            (rc = m_pfm.CloseFile(pfh))) {
        goto err;
    }

    return (rc);

err:
    rc2 = pfh.UnpinPage(pageNum);
    rc2 = m_pfm.CloseFile(pfh);
    return (rc);
}

// DestroyFile
//
// Desc: delete a rm file named 'fileName'
//
RC RM_Manager::DestroyFile(const char *fileName)
{
   return m_pfm.DestroyFile(fileName); 
}

// OpenFile
//
// Desc: open a rm file named 'fileName', save handle into fileHandle
//
RC RM_Manager::OpenFile(const char *fileName, RM_FileHandle &fileHandle)
{
    RC rc, rc2;
    PF_FileHandle pfh;
    PF_PageHandle pph;
    PageNum pageNum;
    char *pData;
    RM_FileHdr *fileHdr;

    // Open page file for record manager
    if ((rc = m_pfm.OpenFile(fileName, pfh)))
        return (rc);

    // AllocatePage for file header
    if ((rc = pfh.GetFirstPage(pph))) {
        m_pfm.CloseFile(pfh);
        return (rc);
    }

    // Get PageNum and pData
    if ((rc = pph.GetPageNum(pageNum)) &&
            (rc = pph.GetData(pData))) {
        goto err;
   }

    // Initialize file header
    fileHdr = (RM_FileHdr*)pData;
    fileHandle.hdr = *fileHdr;
    fileHandle.bHdrChanged = false;
    fileHandle.m_pfh = pfh;

    // MarkDirty and UnpinPage
    if ((rc = pfh.UnpinPage(pageNum)))
        goto err;

    return (rc);

err:
    rc2 = pfh.UnpinPage(pageNum);
    rc2 = m_pfm.CloseFile(pfh);
    return (rc);
}

// CloseFile
//
// Desc: Close file handle
//
RC RM_Manager::CloseFile(RM_FileHandle &fileHandle)
{
    return m_pfm.CloseFile(fileHandle.m_pfh);
}

