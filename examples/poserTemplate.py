import asyncio
import time

from virtualreality import templates



class NullPoser(templates.PoserTemplate):
	def __init__(self, *args, **kwargs):
		super().__init__(*args, **kwargs)

	@templates.thread_register(1/60)
	async def example_thread1(self):
		while self.coro_keepAlive['example_thread1'][0]:
			try:
				self.pose['x'] += 0.0001

				await asyncio.sleep(self.coro_keepAlive['example_thread1'][1])

			except Exception as e:
				print (f'failed: {e}')
				self.coro_keepAlive['example_thread1'][0] = False
				break

	@templates.thread_register(0.1, runInDefaultExecutor=True)
	def example_thread2(self):
		while self.coro_keepAlive['example_thread2'][0]:
			try:
				self.pose['z'] -= 0.01

				time.sleep(self.coro_keepAlive['example_thread2'][1])

			except Exception as e:
				print (f'failed: {e}')
				self.coro_keepAlive['example_thread2'][0] = False
				break

poser = NullPoser()

asyncio.run(poser.main())