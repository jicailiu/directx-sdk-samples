
#pragma once

// These helper functions were copied in due to other files containing them not being WACK-compliant.

// Starts an asynchronous operation and waits on it to complete (or cancel)
// before returning. Pumps messages.

template <typename TIDelegate, typename TIOperation>
HRESULT WaitForCompletion(_In_ TIOperation *pOperation)
{
    Microsoft::WRL::ComPtr<TIOperation> spOperation = pOperation;
    HANDLE hEventCompleted = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
    HRESULT hr = hEventCompleted ? S_OK : HRESULT_FROM_WIN32(GetLastError());
    if (SUCCEEDED(hr))
    {
        Microsoft::WRL::ComPtr<IAsyncInfo> spAsyncInfo;
        hr = spOperation.As(&spAsyncInfo);
        if (SUCCEEDED(hr))
        {
            class FTMEventDelegate :
                public Microsoft::WRL::RuntimeClass<Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::RuntimeClassType::Delegate>, 
                                                    TIDelegate,
                                                    Microsoft::WRL::FtmBase>
            {
            public:
                FTMEventDelegate(_In_ HANDLE hEventCompleted) : _hEventCompleted(hEventCompleted)
                {
                }
                STDMETHOD(Invoke)(_In_ TIOperation *, _In_ AsyncStatus)
                {
                    SetEvent(_hEventCompleted);
                    return S_OK;
                }
            private:
                HANDLE _hEventCompleted;
            };

            auto spCallback = Microsoft::WRL::Make<FTMEventDelegate>(hEventCompleted);
            hr = spCallback ? S_OK : E_OUTOFMEMORY;
            if (SUCCEEDED(hr))
            {
                hr = spOperation->put_Completed(spCallback.Get());
                if (SUCCEEDED(hr))
                {
                    WaitForSingleObjectEx(hEventCompleted, INFINITE, FALSE);
                    spAsyncInfo->get_ErrorCode(&hr);
                }
            }
        }
        CloseHandle(hEventCompleted);
    }
    return hr;
}

inline HRESULT WaitForCompletion(_In_ IAsyncAction *pOperation)
{
    return WaitForCompletion<IAsyncActionCompletedHandler, IAsyncAction>(pOperation);
}
template <typename TIProgressType>
HRESULT WaitForCompletion(_In_ IAsyncActionWithProgress<TIProgressType> *pOperation)
{
    return WaitForCompletion<IAsyncActionWithProgressCompletedHandler<TIProgressType>, IAsyncActionWithProgress<TIProgressType>>(pOperation);
}
template <typename TIOperationResult>
HRESULT WaitForCompletion(_In_ IAsyncOperation<TIOperationResult> *pOperation)
{
    return WaitForCompletion<IAsyncOperationCompletedHandler<TIOperationResult>, IAsyncOperation<TIOperationResult>>(pOperation);
}


// Starts an asynchronous operation and waits on it to complete (or cancel)
// before returning.  Pumps messages.  Presumes the async completion event is
// named put_Completed.
// Also presumes the result method 'GetResults' on the operation interface and attempts to return an
// outbound interface by using this name.

template <typename TIDelegate, typename TIOperation, typename TIResults>
HRESULT WaitForCompletionAndGetResults(_In_ TIOperation *pOperation, _Out_ TIResults *pResults)
{
    HRESULT hr = WaitForCompletion<TIDelegate>(pOperation);
    if (SUCCEEDED(hr))
    {
        hr = pOperation->GetResults(pResults);

        Microsoft::WRL::ComPtr<IAsyncInfo> spAsyncInfo;
        if (SUCCEEDED(pOperation->QueryInterface(IID_PPV_ARGS(&spAsyncInfo))))
        {
            spAsyncInfo->Close();
        }
    }
    return hr;
}
template <typename TIDelegate, typename TIOperation, typename TIResults>
HRESULT WaitForCompletionAndGetResults(_In_ TIOperation *pOperation, _Inout_ Microsoft::WRL::Details::ComPtrRef<Microsoft::WRL::ComPtr<TIResults>> pp)
{
    return WaitForCompletionAndGetResults<TIDelegate, TIOperation>(pOperation, static_cast<TIResults **>(pp));
}
template <typename TIOperationResult, typename TIResults>
HRESULT WaitForCompletionAndGetResults(_In_ IAsyncOperation<TIOperationResult> *pOperation, _Out_ TIResults *pResults)
{
    return WaitForCompletionAndGetResults<IAsyncOperationCompletedHandler<TIOperationResult>, IAsyncOperation<TIOperationResult>, TIResults>(pOperation, pResults);
}
template <typename TIOperationResult, typename TIResults>
HRESULT WaitForCompletionAndGetResults(_In_ IAsyncOperation<TIOperationResult> *pOperation, _Inout_ Microsoft::WRL::Details::ComPtrRef<Microsoft::WRL::ComPtr<TIResults>> pp)
{
    return WaitForCompletionAndGetResults<IAsyncOperationCompletedHandler<TIOperationResult>, IAsyncOperation<TIOperationResult>>(pOperation, static_cast<TIResults **>(pp));
}
template <typename TIOperationResult, typename TIProgressType, typename TIResults>
HRESULT WaitForCompletionAndGetResults(_In_ IAsyncOperationWithProgress<TIOperationResult, TIProgressType> *pOperationWithProgress, _Out_ TIResults *pResults)
{
    return WaitForCompletionAndGetResults<IAsyncOperationWithProgressCompletedHandler<TIOperationResult, TIProgressType>, IAsyncOperationWithProgress<TIOperationResult, TIProgressType>, TIResults>(pOperationWithProgress, pResults);
}
template <typename TIOperationResult, typename TIProgressType, typename TIResults>
HRESULT WaitForCompletionAndGetResults(_In_ IAsyncOperationWithProgress<TIOperationResult, TIProgressType> *pOperationWithProgress, _Inout_ Microsoft::WRL::Details::ComPtrRef<Microsoft::WRL::ComPtr<TIResults>> pp)
{
    return WaitForCompletionAndGetResults<IAsyncOperationWithProgressCompletedHandler<TIOperationResult, TIProgressType>, IAsyncOperationWithProgress<TIOperationResult, TIProgressType>>(pOperationWithProgress, static_cast<TIResults **>(pp));
}

//template <class TAsyncHandler, class TAsyncType>
//class CGetAsyncResults : public TUnknownMT1<TAsyncHandler>
//{
//public:
//    IFACEMETHODIMP Invoke(
//        _In_ TAsyncType* pAsyncAction,
//        AsyncStatus eAsyncStatus
//        ) override
//    {
//        switch (eAsyncStatus)
//        {
//        case AsyncStatus::Started:
//            break;
//
//        case AsyncStatus::Completed:
//            _hr = S_OK;
//            _bIsComplete = true;  // acts as a memory barrier
//            break;
//
//        case AsyncStatus::Canceled:
//            _hr = __HRESULT_FROM_WIN32(ERROR_CANCELLED);
//            _bIsComplete = true; // acts as a memory barrier
//            break;
//
//        case AsyncStatus::Error:
//        {
//            IAsyncInfo* pAsyncInfo;
//            HRESULT hr = pAsyncAction->QueryInterface(IID_PPV_ARGS(&pAsyncInfo));
//            if (SUCCEEDED(hr))
//            {
//                hr = pAsyncInfo->get_ErrorCode(&_hr);
//                pAsyncInfo->Release();
//            }
//
//            if (FAILED(hr))
//            {
//                _hr = hr;
//            }
//
//            if (SUCCEEDED(_hr))
//            {
//                _hr = E_UNEXPECTED;
//            }
//
//            _bIsComplete = true;  // acts as a memory barrier
//            break;
//        }
//        break;
//
//        default:
//            break;
//        }
//
//        return S_OK;
//    }
//
//    // local
//    CGetAsyncResults(
//        )
//        : _bIsComplete(false)
//        , _hr(__HRESULT_FROM_WIN32(ERROR_FUNCTION_NOT_CALLED))
//        , _pAsyncAction(nullptr)
//    {
//    }
//
//    virtual
//    ~CGetAsyncResults(
//        )
//    {
//        if (_pAsyncAction)
//        {
//            _pAsyncAction->Release();
//        }
//    }
//
//     HRESULT Initialize(
//        _In_opt_ TAsyncType* pAsyncAction
//        )
//    {
//        _pAsyncAction = pAsyncAction;
//        if (_pAsyncAction)
//        {
//            _pAsyncAction->AddRef();
//        }
//        return S_OK;
//    }
//
//    _Success_(return == true) bool TryComplete(
//        _Out_ HRESULT* pHr,
//        _Outptr_opt_ TAsyncType** ppAsyncAction
//        )
//    {
//        bool bIsComplete = _bIsComplete; // acts as a memory barrier
//        if (bIsComplete)
//        {
//            *pHr = _hr;
//
//            // If the caller asked to track the async action, they must ask for it here, too
//            ASSERT(!!ppAsyncAction == !!_pAsyncAction);
//
//            if (ppAsyncAction)
//            {
//                *ppAsyncAction = _pAsyncAction; // transfer reference
//                _pAsyncAction = nullptr;
//            }
//        }
//
//        return bIsComplete;
//    }
//
//private:
//    volatile bool _bIsComplete;
//    HRESULT _hr;
//    TAsyncType* _pAsyncAction;
//};