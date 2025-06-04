// 获取cookie用于获取招标的详情信息
var title = "$TITLE"
function getDetailCookie() {
	var element = document.getElementById('infotitle')
	if (element) {
		var elementText = element.textContent.replace(/"/g, '')
		if (elementText == title) {
			return document.cookie
		}
	}
	
	return ''
}

var result = getDetailCookie()
result