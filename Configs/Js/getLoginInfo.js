function getLoginInfo() {
	elements = document.getElementsByClassName('userlogin')
	if (elements.length == 0) {
		return ''
	}
	
	return document.cookie
}

var result = getLoginInfo()
result