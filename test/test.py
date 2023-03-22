import asyncio
from tqdm import trange


async def cl(p):
    r, w = await asyncio.open_connection('localhost', 6668)
    b = bytes("1145141919810", 'utf-8')
    for i in trange(100000, position=p):
        for k in range(10):
            for j in range(10):
                w.write(b)
            await w.drain()
        # await asyncio.sleep(0.1)

    w.close()


async def main():
    tasks = [asyncio.create_task(cl(i)) for i in range(100)]
    await asyncio.gather(*tasks)

asyncio.run(main())
