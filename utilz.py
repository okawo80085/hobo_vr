import asyncio

def convv(sss):
	if len(sss) < 1:
		return ''.encode('utf-8')

	if sss[-1] != '\n':
		return (sss + '\n').encode('utf-8')

	return sss.encode('utf-8')

async def newRead(reader):
	data = []
	temp = None
	while temp != '\n' and temp != '':
		temp = await reader.read(1)
		temp = temp.decode('utf-8')
		data.append(temp)

	return ''.join(data)