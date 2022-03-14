/**
* HTTPÍøÂç·µ»ØÂë
*/
#ifndef W_HTTP_ERROR_CODE_H
#define W_HTTP_ERROR_CODE_H

namespace wang {

	enum EHttpResponseCode
	{
		EHRCode_Succ = 200,
		EHRCode_Timeout = 801,
		EHRCode_Resolve = 802,
		EHRCode_Connect = 803,
		EHRCode_Write = 804,
		EHRCode_Err = 805,
		EHRCode_Exception = 806,
	};
}
#endif
