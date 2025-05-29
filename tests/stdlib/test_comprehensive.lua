-- Comprehensive test cases for RangeLua standard library integration
-- This file tests integration between multiple stdlib modules

print("=== RangeLua Standard Library Integration Test Suite ===")
print()

-- Test 1: String and Math integration
print("Test 1: String and Math integration")
local numbers = {"3.14", "2.71", "1.41", "0.57"}
print("String numbers:", table.concat(numbers, ", "))

local sum = 0
for i, str_num in ipairs(numbers) do
    local num = tonumber(str_num)
    if num then
        sum = sum + num
        print("  " .. str_num .. " -> " .. tostring(num))
    end
end
print("Sum:", sum)
print("Average:", sum / #numbers)
print("Formatted average:", string.format("%.3f", sum / #numbers))
print()

-- Test 2: Table and String processing
print("Test 2: Table and String processing")
local text = "The quick brown fox jumps over the lazy dog"
local words = {}

-- Split text into words (simple implementation)
for word in string.gmatch(text, "%S+") do
    table.insert(words, word)
end

print("Original text:", text)
print("Word count:", #words)
print("Words:", table.concat(words, " | "))

-- Process words
local processed = {}
for i, word in ipairs(words) do
    local processed_word = string.upper(string.sub(word, 1, 1)) .. string.lower(string.sub(word, 2))
    table.insert(processed, processed_word)
end
print("Processed:", table.concat(processed, " "))
print()

-- Test 3: Mathematical calculations with formatting
print("Test 3: Mathematical calculations with formatting")
local angles = {0, 30, 45, 60, 90}
print("Trigonometric table:")
print("Angle\tSin\t\tCos\t\tTan")
print("-----\t---\t\t---\t\t---")

for i, deg in ipairs(angles) do
    local rad = math.rad(deg)
    local sin_val = math.sin(rad)
    local cos_val = math.cos(rad)
    local tan_val = math.tan(rad)
    
    print(string.format("%d°\t%.4f\t\t%.4f\t\t%.4f", 
          deg, sin_val, cos_val, tan_val))
end
print()

-- Test 4: Data analysis with all libraries
print("Test 4: Data analysis simulation")
math.randomseed(42)  -- For reproducible results

-- Generate sample data
local data = {}
for i = 1, 20 do
    table.insert(data, math.random(1, 100))
end

print("Sample data (20 random numbers 1-100):")
print(table.concat(data, ", "))

-- Calculate statistics
local sum = 0
local min_val = math.huge
local max_val = -math.huge

for i, value in ipairs(data) do
    sum = sum + value
    min_val = math.min(min_val, value)
    max_val = math.max(max_val, value)
end

local mean = sum / #data
local sorted_data = {table.unpack(data)}
table.sort(sorted_data)

print("\nStatistics:")
print("  Count:", #data)
print("  Sum:", sum)
print("  Mean:", string.format("%.2f", mean))
print("  Min:", min_val)
print("  Max:", max_val)
print("  Range:", max_val - min_val)
print("  Median:", sorted_data[math.ceil(#sorted_data / 2)])
print("  Sorted:", table.concat(sorted_data, ", "))
print()

-- Test 5: String manipulation with table operations
print("Test 5: Advanced string manipulation")
local sentences = {
    "Hello world",
    "Lua is powerful",
    "Programming is fun",
    "RangeLua rocks"
}

print("Original sentences:")
for i, sentence in ipairs(sentences) do
    print("  " .. i .. ": " .. sentence)
end

-- Reverse words in each sentence
local reversed_sentences = {}
for i, sentence in ipairs(sentences) do
    local words = {}
    for word in string.gmatch(sentence, "%S+") do
        table.insert(words, 1, word)  -- Insert at beginning to reverse
    end
    table.insert(reversed_sentences, table.concat(words, " "))
end

print("\nReversed word order:")
for i, sentence in ipairs(reversed_sentences) do
    print("  " .. i .. ": " .. sentence)
end

-- Character frequency analysis
local char_count = {}
local total_chars = 0

for i, sentence in ipairs(sentences) do
    for j = 1, string.len(sentence) do
        local char = string.sub(sentence, j, j)
        if char ~= " " then  -- Skip spaces
            char = string.lower(char)
            char_count[char] = (char_count[char] or 0) + 1
            total_chars = total_chars + 1
        end
    end
end

print("\nCharacter frequency (top 10):")
local char_pairs = {}
for char, count in pairs(char_count) do
    table.insert(char_pairs, {char, count})
end

table.sort(char_pairs, function(a, b) return a[2] > b[2] end)

for i = 1, math.min(10, #char_pairs) do
    local char, count = char_pairs[i][1], char_pairs[i][2]
    local percentage = (count / total_chars) * 100
    print(string.format("  '%s': %d (%.1f%%)", char, count, percentage))
end
print()

-- Test 6: Error handling and type checking
print("Test 6: Error handling and type checking")
local test_values = {42, "hello", 3.14, true, nil, {}, print}

print("Type analysis:")
for i, value in ipairs(test_values) do
    local value_type = type(value)
    local value_str = tostring(value)
    local math_type = math.type(value)
    
    print(string.format("  Value %d: %s (type: %s, math.type: %s)", 
          i, value_str, value_type, tostring(math_type)))
end

-- Safe number conversion
print("\nSafe number conversions:")
local test_strings = {"42", "3.14", "not_a_number", "0xFF", ""}
for i, str in ipairs(test_strings) do
    local num = tonumber(str)
    if num then
        print(string.format("  '%s' -> %.2f (sqrt: %.2f)", str, num, math.sqrt(math.abs(num))))
    else
        print(string.format("  '%s' -> conversion failed", str))
    end
end
print()

-- Test 7: Performance comparison
print("Test 7: Performance comparison")
local start_time = os and os.clock and os.clock() or 0

-- String concatenation vs table.concat
local parts = {}
for i = 1, 1000 do
    table.insert(parts, tostring(i))
end

local concat_result = table.concat(parts, ",")
print("table.concat with 1000 elements: length =", string.len(concat_result))

-- Mathematical operations
local result = 0
for i = 1, 1000 do
    result = result + math.sin(i) * math.cos(i)
end
print("Mathematical operations result:", string.format("%.6f", result))
print()

-- Test 8: Complex data structure manipulation
print("Test 8: Complex data structures")
local students = {
    {name = "Alice", scores = {85, 92, 78, 96}},
    {name = "Bob", scores = {76, 84, 88, 82}},
    {name = "Charlie", scores = {94, 89, 91, 87}},
    {name = "Diana", scores = {88, 95, 83, 90}}
}

print("Student grade analysis:")
for i, student in ipairs(students) do
    local total = 0
    for j, score in ipairs(student.scores) do
        total = total + score
    end
    local average = total / #student.scores
    local grade = average >= 90 and "A" or average >= 80 and "B" or average >= 70 and "C" or "F"
    
    print(string.format("  %s: %.1f average (Grade %s)", student.name, average, grade))
    print("    Scores: " .. table.concat(student.scores, ", "))
end
print()

print("=== Standard Library Integration Tests Complete ===")
