import { expect } from 'chai'
import sum from '../src/app.js'

describe('src/app.js', () => {
  it('should add 1 + 1 to make two', () => {
    let result = sum(1, 1)
    expect(result).to.equal(2)
  })

  it('should add 1 + 2 to make three', () => {
    let result = sum(1, 2)
    expect(result).to.equal(3)
  })

  it('should add 1 + 4 to make five', () => {
    let result = sum(1, 4)
    expect(result).to.equal(5)
  })
})
