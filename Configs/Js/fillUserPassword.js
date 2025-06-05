// 自动填写用户名密码
var username = "$USERNAME"
var password = "$PASSWORD"

function fillUserPassword() {
	var element = document.getElementById('loginUserId')
	if (element) {
		element.value = username
		element = document.getElementById('loginPassword')
		if (element) {
			element.value = password
			return 1
		}
	}
	
	return 0
}

var result = fillUserPassword()
result